#include <array>
#include <chrono>
#include <fstream>
#include <iostream>

#include "constant_mappings.hpp"
#include "logical_query_plan/logical_plan_root_node.hpp"
#include "logical_query_plan/lqp_translator.hpp"
#include "operators/join_hash.hpp"
#include "operators/join_sort_merge.hpp"
#include "operators/product.hpp"
#include "operators/table_scan.hpp"
#include "operators/table_wrapper.hpp"
#include "operators/utils/flatten_pqp.hpp"
#include "optimizer/join_ordering/abstract_join_plan_node.hpp"
#include "optimizer/join_ordering/cost_model_naive.hpp"
#include "optimizer/join_ordering/cost_model_segmented.hpp"
#include "optimizer/join_ordering/dp_ccp.hpp"
#include "optimizer/join_ordering/dp_ccp_top_k.hpp"
#include "optimizer/strategy/join_ordering_rule.hpp"
#include "optimizer/table_statistics.hpp"
#include "planviz/lqp_visualizer.hpp"
#include "planviz/sql_query_plan_visualizer.hpp"
#include "scheduler/current_scheduler.hpp"
#include "sql/sql_pipeline_statement.hpp"
#include "sql/sql.hpp"
#include "storage/storage_manager.hpp"
#include "tpch/tpch_db_generator.hpp"
#include "tpch/tpch_queries.hpp"
#include "utils/timer.hpp"
#include "utils/table_generator2.hpp"

using namespace std::string_literals;  // NOLINT
using namespace opossum;  // NOLINT

int main() {
  std::cout << "- Hyrise Join Ordering Evaluator" << std::endl;

  const auto scale_factor = 0.005f;
  const auto sql = tpch_queries[4];
  const auto name = "TPCH-5"s;

  auto plan_durations = std::vector<std::chrono::microseconds>();

  TpchDbGenerator{scale_factor}.generate_and_store();

  auto pipeline_statement = SQL{sql}.disable_mvcc().pipeline_statement();

  const auto lqp = pipeline_statement.get_optimized_logical_plan();
  const auto lqp_root = LogicalPlanRootNode::make(lqp);
  const auto join_graph = JoinGraph::from_lqp(lqp);

  const auto cost_model = std::make_shared<CostModelSegmented>();

  DpCcpTopK dp_ccp_top_k{DpSubplanCacheTopK::NO_ENTRY_LIMIT, cost_model};

  dp_ccp_top_k(join_graph);

  JoinVertexSet all_vertices{join_graph->vertices.size()};
  all_vertices.flip();
  const auto join_plans = dp_ccp_top_k.subplan_cache()->get_best_plans(all_vertices);

  std::cout << "-- Generated plans: " << join_plans.size() << std::endl;

  auto current_plan_idx = 0;

  for (const auto& join_plan : join_plans) {
    const auto join_ordered_sub_lqp = join_plan->to_lqp();
    for (const auto& parent_relation : join_graph->output_relations) {
      parent_relation.output->set_input(parent_relation.input_side, join_ordered_sub_lqp);
    }

    const auto pqp = LQPTranslator{}.translate_node(lqp_root->left_input());

    SQLQueryPlan plan;
    plan.add_tree_by_root(pqp);

    Timer timer;
    CurrentScheduler::schedule_and_wait_for_tasks(plan.create_tasks());
    const auto plan_duration = timer.lap();
    plan_durations.emplace_back(plan_duration);

    const auto operators = flatten_pqp(pqp);

    /**
     * Visualize
     */
    GraphvizConfig graphviz_config;
    graphviz_config.format = "svg";
    VizGraphInfo viz_graph_info;
    viz_graph_info.bg_color = "black";

    SQLQueryPlanVisualizer visualizer{graphviz_config, viz_graph_info, {}, {}};
    visualizer.set_cost_model(cost_model);
    visualizer.visualize(plan, "tmp.dot", std::string("viz/") + name + "_" + std::to_string(current_plan_idx) + "_" + std::to_string(plan_duration.count()) + ".svg");

    /**
     * CSV
     */
    auto csv = std::ofstream{std::string("viz/") + name + ".csv"};
    for (auto plan_idx = size_t{0}; plan_idx < plan_durations.size(); ++plan_idx) {
      csv << plan_idx << "," << plan_durations[plan_idx].count() << "\n";
    }
    csv.close();

    ++current_plan_idx;
  }
}