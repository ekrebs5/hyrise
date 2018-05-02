#include "../../../base_test.hpp"
#include "operators/jit_operator/operators/jit_read_tuple.hpp"
#include "operators/jit_operator/operators/jit_write_tuple.hpp"
#include "utils/load_table.hpp"

namespace opossum {

class JitReadWriteTupleTest : public BaseTest {};

TEST_F(JitReadWriteTupleTest, CreateOutputTable) {
  auto write_tuple = std::make_shared<JitWriteTuple>();

  TableColumnDefinitions column_definitions = {{"a", DataType::Int, false},
                                               {"b", DataType::Long, true},
                                               {"c", DataType::Float, false},
                                               {"d", DataType::Double, false},
                                               {"e", DataType::String, true}};

  for (const auto& column_definition : column_definitions) {
    write_tuple->add_output_column(column_definition.name,
                                   JitTupleValue(column_definition.data_type, column_definition.nullable, 0));
  }

  auto output_table = write_tuple->create_output_table(1);
  ASSERT_EQ(output_table->column_definitions(), column_definitions);
}

TEST_F(JitReadWriteTupleTest, TupleIndicesAreIncremented) {
  auto read_tuple = std::make_shared<JitReadTuple>();

  // Add different kinds of values (input columns, literals, temporary values) to the runtime tuple
  auto tuple_index_1 = read_tuple->add_input_column(DataType::Int, false, ColumnID{0}).tuple_index();
  auto tuple_index_2 = read_tuple->add_literal_value(1).tuple_index();
  auto tuple_index_3 = read_tuple->add_temporary_value();
  auto tuple_index_4 = read_tuple->add_input_column(DataType::Int, false, ColumnID{1}).tuple_index();
  auto tuple_index_5 = read_tuple->add_literal_value("some string").tuple_index();
  auto tuple_index_6 = read_tuple->add_temporary_value();

  // All values should have their own position in the tuple with tuple indices increasing
  ASSERT_LT(tuple_index_1, tuple_index_2);
  ASSERT_LT(tuple_index_2, tuple_index_3);
  ASSERT_LT(tuple_index_3, tuple_index_4);
  ASSERT_LT(tuple_index_4, tuple_index_5);
  ASSERT_LT(tuple_index_5, tuple_index_6);

  // Adding the same input column twice should not create a new value in the tuple
  auto tuple_index_1_b = read_tuple->add_input_column(DataType::Int, false, ColumnID{0}).tuple_index();
  ASSERT_EQ(tuple_index_1, tuple_index_1_b);
}

TEST_F(JitReadWriteTupleTest, LiteralValuesAreInitialized) {
  auto read_tuple = std::make_shared<JitReadTuple>();

  auto int_value = read_tuple->add_literal_value(1);
  auto float_value = read_tuple->add_literal_value(1.23f);
  auto double_value = read_tuple->add_literal_value(12.3);
  auto string_value = read_tuple->add_literal_value("some string");

  // Since we only test literal values here an empty input table is sufficient
  JitRuntimeContext context;
  Table input_table(TableColumnDefinitions{}, TableType::Data);
  read_tuple->before_query(input_table, context);

  ASSERT_EQ(int_value.get<int32_t>(context), 1);
  ASSERT_EQ(float_value.get<float>(context), 1.23f);
  ASSERT_EQ(double_value.get<double>(context), 12.3);
  ASSERT_EQ(string_value.get<std::string>(context), "some string");
}

TEST_F(JitReadWriteTupleTest, CopyTable) {
  JitRuntimeContext context;

  // Create operator chain that passes from the input tuple to an output table unmodified
  auto read_tuple = std::make_shared<JitReadTuple>();
  auto write_tuple = std::make_shared<JitWriteTuple>();
  read_tuple->set_next_operator(write_tuple);

  // Add all input table columns to pipeline
  auto a_value = read_tuple->add_input_column(DataType::Int, true, ColumnID{0});
  auto b_value = read_tuple->add_input_column(DataType::Float, true, ColumnID{1});
  write_tuple->add_output_column("a", a_value);
  write_tuple->add_output_column("b", b_value);

  // Initialize operators with actual input table
  auto input_table = load_table("src/test/tables/int_float_null_sorted_asc.tbl", 2);
  auto output_table = write_tuple->create_output_table(2);
  read_tuple->before_query(*input_table, context);
  write_tuple->before_query(*output_table, context);

  // Pass each chunk through the pipeline
  for (const auto& chunk : input_table->chunks()) {
    read_tuple->before_chunk(*input_table, *chunk, context);
    read_tuple->execute(context);
    write_tuple->after_chunk(*output_table, context);
  }
  write_tuple->after_query(*output_table, context);

  // Both tables should be equal now
  ASSERT_TRUE(check_table_equal(input_table, output_table, OrderSensitivity::Yes, TypeCmpMode::Strict,
                                FloatComparisonMode::AbsoluteDifference));
}

}  // namespace opossum
