#pragma once

#include <memory>
#include <optional>
#include <ostream>
#include <string>

#include "all_type_variant.hpp"
#include "base_column_statistics.hpp"

namespace opossum {

/**
 * @tparam ColumnDataType   the DataType of the values in the Column that these statistics represent
 */
template <typename ColumnDataType>
class MinimalColumnStatistics : public BaseColumnStatistics {
 public:
  // To be used for columns for which MinimalColumnStatistics can't be computed
  static MinimalColumnStatistics dummy();

  MinimalColumnStatistics(const float null_value_ratio, const float distinct_count, const ColumnDataType min,
                          const ColumnDataType max);

  /**
   * @defgroup Member access
   * @{
   */
  ColumnDataType min() const;
  ColumnDataType max() const;
  float distinct_count() const override;
  /** @} */

  /**
   * @defgroup Implementations for BaseColumnStatistics
   * @{
   */
  std::shared_ptr<BaseColumnStatistics> clone() const override;
  FilterByValueEstimate estimate_predicate_with_value(
      const PredicateCondition predicate_condition, const AllTypeVariant& variant_value,
      const std::optional<AllTypeVariant>& value2 = std::nullopt) const override;

  FilterByValueEstimate estimate_predicate_with_value_placeholder(
      const PredicateCondition predicate_condition,
      const std::optional<AllTypeVariant>& value2 = std::nullopt) const override;

  FilterByColumnComparisonEstimate estimate_predicate_with_column(
      const PredicateCondition predicate_condition,
      const std::shared_ptr<const BaseColumnStatistics>& base_right_column_statistics) const override;

  /** @} */

  /**
   * @defgroup Cardinality Estimation helpers
   * @{
   */

  /**
   * @return the ratio of rows of this Column that are in the range [minimum, maximum]
   */
  float estimate_range_selectivity(const ColumnDataType minimum, const ColumnDataType maximum) const;

  /**
   * @return estimate the predicate `column BETWEEN minimum AND maximum`
   */
  FilterByValueEstimate estimate_range(const ColumnDataType minimum, const ColumnDataType maximum) const;

  /**
   * @return estimate the predicate `column = value`
   */
  FilterByValueEstimate estimate_equals_with_value(const ColumnDataType value) const;

  /**
   * @return estimate the predicate `column != value`
   */
  FilterByValueEstimate estimate_not_equals_with_value(const ColumnDataType value) const;
  /** @} */

 protected:
  std::string _description() const override;

 private:
  ColumnDataType _min;
  ColumnDataType _max;
  float _distinct_count;
};

}  // namespace opossum