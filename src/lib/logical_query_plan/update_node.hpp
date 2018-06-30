#pragma once

#include <memory>
#include <string>
#include <vector>

#include "abstract_lqp_node.hpp"

namespace opossum {

class AbstractExpression;

/**
 * Node type to represent updates (i.e., invalidation and inserts) in a table.
 */
class UpdateNode : public EnableMakeForLQPNode<UpdateNode>, public AbstractLQPNode {
 public:
  UpdateNode(const std::string& table_name,
                      const std::vector<std::shared_ptr<AbstractExpression>>& update_column_expressions);

  std::string description() const override;
  std::vector<std::shared_ptr<AbstractExpression>> node_expressions() const override;

  const std::string table_name;
  const std::vector<std::shared_ptr<AbstractExpression>> update_column_expressions;

 protected:
  std::shared_ptr<AbstractLQPNode> _shallow_copy_impl(LQPNodeMapping & node_mapping) const override;
  bool _shallow_equals_impl(const AbstractLQPNode& rhs, const LQPNodeMapping & node_mapping) const override;
};

}  // namespace opossumF
