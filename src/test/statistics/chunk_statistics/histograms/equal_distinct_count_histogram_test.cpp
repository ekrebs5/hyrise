#include <limits>
#include <memory>
#include <string>

#include "base_test.hpp"
#include "gtest/gtest.h"

#include "statistics/chunk_statistics/histograms/equal_distinct_count_histogram.hpp"
#include "statistics/chunk_statistics/histograms/generic_histogram.hpp"
#include "statistics/chunk_statistics/histograms/histogram_utils.hpp"
#include "utils/load_table.hpp"

namespace opossum {

class EqualDistinctCountHistogramTest : public BaseTest {
  void SetUp() override {
    _int_float4 = load_table("src/test/tables/int_float4.tbl");
    _float2 = load_table("src/test/tables/float2.tbl");
    _string2 = load_table("src/test/tables/string2.tbl");
    _string3 = load_table("src/test/tables/string3.tbl");
    _string_with_prefix = load_table("src/test/tables/string_with_prefix.tbl");
    _string_like_pruning = load_table("src/test/tables/string_like_pruning.tbl");
  }

 protected:
  std::shared_ptr<Table> _int_float4;
  std::shared_ptr<Table> _float2;
  std::shared_ptr<Table> _string2;
  std::shared_ptr<Table> _string3;
  std::shared_ptr<Table> _string_with_prefix;
  std::shared_ptr<Table> _string_like_pruning;
};

TEST_F(EqualDistinctCountHistogramTest, Basic) {
  const auto hist = EqualDistinctCountHistogram<int32_t>::from_segment(
      this->_int_float4->get_chunk(ChunkID{0})->get_segment(ColumnID{0}), 2u);

  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, AllTypeVariant{0}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, 0).first, 0.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, AllTypeVariant{12}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, 12).first, 1.f);

  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, AllTypeVariant{1'234}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, 1'234).first, 0.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, AllTypeVariant{123'456}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, 123'456).first, 2.5f);

  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, AllTypeVariant{1'000'000}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, 1'000'000).first, 0.f);
}

TEST_F(EqualDistinctCountHistogramTest, UnevenBins) {
  auto hist = EqualDistinctCountHistogram<int32_t>::from_segment(
      _int_float4->get_chunk(ChunkID{0})->get_segment(ColumnID{0}), 3u);

  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, AllTypeVariant{0}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, 0).first, 0.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, AllTypeVariant{12}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, 12).first, 1.f);

  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, AllTypeVariant{1'234}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, 1'234).first, 0.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, AllTypeVariant{123'456}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, 123'456).first, 3.f);

  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, AllTypeVariant{1'000'000}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, 1'000'000).first, 0.f);
}

TEST_F(EqualDistinctCountHistogramTest, Float) {
  auto hist =
      EqualDistinctCountHistogram<float>::from_segment(_float2->get_chunk(ChunkID{0})->get_segment(ColumnID{0}), 3u);

  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, AllTypeVariant{0.4f}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, 0.4f).first, 0.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, AllTypeVariant{0.5f}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, 0.5f).first, 4 / 4.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, AllTypeVariant{1.1f}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, 1.1f).first, 4 / 4.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, AllTypeVariant{1.3f}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, 1.3f).first, 4 / 4.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, AllTypeVariant{2.2f}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, 2.2f).first, 4 / 4.f);

  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, AllTypeVariant{2.3f}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, 2.3f).first, 0.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, AllTypeVariant{2.5f}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, 2.5f).first, 6 / 3.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, AllTypeVariant{2.9f}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, 2.9f).first, 6 / 3.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, AllTypeVariant{3.3f}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, 3.3f).first, 6 / 3.f);

  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, AllTypeVariant{3.5f}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, 3.5f).first, 0.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, AllTypeVariant{3.6f}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, 3.6f).first, 4 / 3.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, AllTypeVariant{3.9f}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, 3.9f).first, 4 / 3.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, AllTypeVariant{6.1f}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, 6.1f).first, 4 / 3.f);

  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, AllTypeVariant{6.2f}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, 6.2f).first, 0.f);
}

TEST_F(EqualDistinctCountHistogramTest, String) {
  auto hist = EqualDistinctCountHistogram<std::string>::from_segment(
      _string2->get_chunk(ChunkID{0})->get_segment(ColumnID{0}), 4u);

  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, "a"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, "a").first, 0.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "aa"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, "aa").first, 3 / 3.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "ab"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, "ab").first, 3 / 3.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "b"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, "b").first, 3 / 3.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "birne"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, "birne").first, 3 / 3.f);

  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, "biscuit"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, "biscuit").first, 0.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "bla"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, "bla").first, 4 / 3.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "blubb"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, "blubb").first, 4 / 3.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "bums"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, "bums").first, 4 / 3.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "ttt"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, "ttt").first, 4 / 3.f);

  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, "turkey"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, "turkey").first, 0.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "uuu"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, "uuu").first, 4 / 3.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "vvv"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, "vvv").first, 4 / 3.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "www"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, "www").first, 4 / 3.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "xxx"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, "xxx").first, 4 / 3.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "yyy"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, "yyy").first, 4 / 2.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "zzz"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, "zzz").first, 4 / 2.f);

  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, "zzzzzz"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Equals, "zzzzzz").first, 0.f);
}

TEST_F(EqualDistinctCountHistogramTest, StringPruning) {
  /**
   * 4 bins
   *  [aa, b, birne]    -> [aa, bir]
   *  [bla, bums, ttt]  -> [bla, ttt]
   *  [uuu, www, xxx]   -> [uuu, xxx]
   *  [yyy, zzz]        -> [yyy, zzz]
   */
  auto hist = EqualDistinctCountHistogram<std::string>::from_segment(
      _string2->get_chunk(ChunkID{0})->get_segment(ColumnID{0}), 4u, "abcdefghijklmnopqrstuvwxyz", 3u);

  // These values are smaller than values in bin 0.
  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, ""));
  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, "a"));

  // These values fall within bin 0.
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "aa"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "aaa"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "b"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "bir"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "bira"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "birne"));

  // These values are between bin 0 and 1.
  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, "birnea"));
  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, "bis"));
  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, "biscuit"));
  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, "bja"));
  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, "bk"));
  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, "bkz"));
  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, "bl"));

  // These values fall within bin 1.
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "bla"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "c"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "mmopasdasdasd"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "s"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "t"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "tt"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "ttt"));

  // These values are between bin 1 and 2.
  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, "ttta"));
  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, "tttzzzzz"));
  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, "turkey"));
  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, "uut"));
  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, "uutzzzzz"));

  // These values fall within bin 2.
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "uuu"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "uuuzzz"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "uv"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "uvz"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "v"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "w"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "wzzzzzzzzz"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "x"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "xxw"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "xxx"));

  // These values are between bin 2 and 3.
  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, "xxxa"));
  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, "xxxzzzzzz"));
  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, "xy"));
  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, "xyzz"));
  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, "y"));
  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, "yyx"));
  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, "yyxzzzzz"));

  // These values fall within bin 3.
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "yyy"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "yyyzzzzz"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "yz"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "z"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Equals, "zzz"));

  // These values are greater than the upper bound of the histogram.
  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, "zzza"));
  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Equals, "zzzzzzzzz"));
}

TEST_F(EqualDistinctCountHistogramTest, LessThan) {
  auto hist = EqualDistinctCountHistogram<int32_t>::from_segment(
      _int_float4->get_chunk(ChunkID{0})->get_segment(ColumnID{0}), 3u);

  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::LessThan, AllTypeVariant{12}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, 12).first, 0.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, AllTypeVariant{70}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, 70).first, (70.f - 12) / (123 - 12 + 1) * 2);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, AllTypeVariant{1'234}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, 1'234).first, 2.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, AllTypeVariant{12'346}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, 12'346).first, 4.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, AllTypeVariant{123'456}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, 123'456).first, 4.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, AllTypeVariant{123'457}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, 123'457).first, 7.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, AllTypeVariant{1'000'000}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, 1'000'000).first, 7.f);
}

TEST_F(EqualDistinctCountHistogramTest, FloatLessThan) {
  auto hist =
      EqualDistinctCountHistogram<float>::from_segment(_float2->get_chunk(ChunkID{0})->get_segment(ColumnID{0}), 3u);

  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::LessThan, AllTypeVariant{0.5f}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, 0.5f).first, 0.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, AllTypeVariant{1.0f}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, 1.0f).first,
                  (1.0f - 0.5f) / std::nextafter(2.2f - 0.5f, std::numeric_limits<float>::infinity()) * 4);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, AllTypeVariant{1.7f}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, 1.7f).first,
                  (1.7f - 0.5f) / std::nextafter(2.2f - 0.5f, std::numeric_limits<float>::infinity()) * 4);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan,
                                      AllTypeVariant{std::nextafter(2.2f, std::numeric_limits<float>::infinity())}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan,
                                             std::nextafter(2.2f, std::numeric_limits<float>::infinity()))
                      .first,
                  4.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, AllTypeVariant{2.5f}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, 2.5f).first, 4.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, AllTypeVariant{3.0f}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, 3.0f).first,
                  4.f + (3.0f - 2.5f) / std::nextafter(3.3f - 2.5f, std::numeric_limits<float>::infinity()) * 6);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, AllTypeVariant{3.3f}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, 3.3f).first,
                  4.f + (3.3f - 2.5f) / std::nextafter(3.3f - 2.5f, std::numeric_limits<float>::infinity()) * 6);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan,
                                      AllTypeVariant{std::nextafter(3.3f, std::numeric_limits<float>::infinity())}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan,
                                             std::nextafter(3.3f, std::numeric_limits<float>::infinity()))
                      .first,
                  4.f + 6.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, AllTypeVariant{3.6f}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, 3.6f).first, 4.f + 6.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, AllTypeVariant{3.9f}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, 3.9f).first,
                  4.f + 6.f + (3.9f - 3.6f) / std::nextafter(6.1f - 3.6f, std::numeric_limits<float>::infinity()) * 4);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, AllTypeVariant{5.9f}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, 5.9f).first,
                  4.f + 6.f + (5.9f - 3.6f) / std::nextafter(6.1f - 3.6f, std::numeric_limits<float>::infinity()) * 4);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan,
                                      AllTypeVariant{std::nextafter(6.1f, std::numeric_limits<float>::infinity())}));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan,
                                             std::nextafter(6.1f, std::numeric_limits<float>::infinity()))
                      .first,
                  4.f + 6.f + 4.f);
}

TEST_F(EqualDistinctCountHistogramTest, StringLessThan) {
  auto hist = EqualDistinctCountHistogram<std::string>::from_segment(
      _string3->get_chunk(ChunkID{0})->get_segment(ColumnID{0}), 4u, "abcdefghijklmnopqrstuvwxyz", 4u);

  // "abcd"
  const auto bin_1_lower = 0 * (ipow(26, 3) + ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 +
                           1 * (ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 + 2 * (ipow(26, 1) + ipow(26, 0)) + 1 +
                           3 * (ipow(26, 0)) + 1;
  // "efgh"
  const auto bin_1_upper = 4 * (ipow(26, 3) + ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 +
                           5 * (ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 + 6 * (ipow(26, 1) + ipow(26, 0)) + 1 +
                           7 * (ipow(26, 0)) + 1;
  // "ijkl"
  const auto bin_2_lower = 8 * (ipow(26, 3) + ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 +
                           9 * (ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 + 10 * (ipow(26, 1) + ipow(26, 0)) + 1 +
                           11 * (ipow(26, 0)) + 1;
  // "mnop"
  const auto bin_2_upper = 12 * (ipow(26, 3) + ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 +
                           13 * (ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 + 14 * (ipow(26, 1) + ipow(26, 0)) + 1 +
                           15 * (ipow(26, 0)) + 1;
  // "oopp"
  const auto bin_3_lower = 14 * (ipow(26, 3) + ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 +
                           14 * (ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 + 15 * (ipow(26, 1) + ipow(26, 0)) + 1 +
                           15 * (ipow(26, 0)) + 1;
  // "qrst"
  const auto bin_3_upper = 16 * (ipow(26, 3) + ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 +
                           17 * (ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 + 18 * (ipow(26, 1) + ipow(26, 0)) + 1 +
                           19 * (ipow(26, 0)) + 1;
  // "uvwx"
  const auto bin_4_lower = 20 * (ipow(26, 3) + ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 +
                           21 * (ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 + 22 * (ipow(26, 1) + ipow(26, 0)) + 1 +
                           23 * (ipow(26, 0)) + 1;
  // "yyzz"
  const auto bin_4_upper = 24 * (ipow(26, 3) + ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 +
                           24 * (ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 + 25 * (ipow(26, 1) + ipow(26, 0)) + 1 +
                           25 * (ipow(26, 0)) + 1;

  const auto bin_1_width = (bin_1_upper - bin_1_lower + 1.f);
  const auto bin_2_width = (bin_2_upper - bin_2_lower + 1.f);
  const auto bin_3_width = (bin_3_upper - bin_3_lower + 1.f);
  const auto bin_4_width = (bin_4_upper - bin_4_lower + 1.f);

  constexpr auto bin_1_count = 4.f;
  constexpr auto bin_2_count = 6.f;
  constexpr auto bin_3_count = 3.f;
  constexpr auto bin_4_count = 3.f;
  constexpr auto total_count = bin_1_count + bin_2_count + bin_3_count + bin_4_count;

  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::LessThan, "aaaa"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "aaaa").first, 0.f);

  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::LessThan, "abcd"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "abcd").first, 0.f);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, "abce"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "abce").first,
                  1 / bin_1_width * bin_1_count);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, "abcf"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "abcf").first,
                  2 / bin_1_width * bin_1_count);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, "abcf"));
  EXPECT_FLOAT_EQ(
      hist->estimate_cardinality(PredicateCondition::LessThan, "cccc").first,
      (2 * (ipow(26, 3) + ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 + 2 * (ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) +
       1 + 2 * (ipow(26, 1) + ipow(26, 0)) + 1 + 2 * (ipow(26, 0)) + 1 - bin_1_lower) /
          bin_1_width * bin_1_count);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, "dddd"));
  EXPECT_FLOAT_EQ(
      hist->estimate_cardinality(PredicateCondition::LessThan, "dddd").first,
      (3 * (ipow(26, 3) + ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 + 3 * (ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) +
       1 + 3 * (ipow(26, 1) + ipow(26, 0)) + 1 + 3 * (ipow(26, 0)) + 1 - bin_1_lower) /
          bin_1_width * bin_1_count);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, "efgg"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "efgg").first,
                  (bin_1_width - 2) / bin_1_width * bin_1_count);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, "efgh"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "efgh").first,
                  (bin_1_width - 1) / bin_1_width * bin_1_count);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, "efgi"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "efgi").first, bin_1_count);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, "ijkl"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "ijkl").first, bin_1_count);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, "ijkm"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "ijkm").first,
                  1 / bin_2_width * bin_2_count + bin_1_count);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, "ijkn"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "ijkn").first,
                  2 / bin_2_width * bin_2_count + bin_1_count);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, "jjjj"));
  EXPECT_FLOAT_EQ(
      hist->estimate_cardinality(PredicateCondition::LessThan, "jjjj").first,
      (9 * (ipow(26, 3) + ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 + 9 * (ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) +
       1 + 9 * (ipow(26, 1) + ipow(26, 0)) + 1 + 9 * (ipow(26, 0)) + 1 - bin_2_lower) /
              bin_2_width * bin_2_count +
          bin_1_count);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, "kkkk"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "kkkk").first,
                  (10 * (ipow(26, 3) + ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 +
                   10 * (ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 + 10 * (ipow(26, 1) + ipow(26, 0)) + 1 +
                   10 * (ipow(26, 0)) + 1 - bin_2_lower) /
                          bin_2_width * bin_2_count +
                      bin_1_count);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, "lzzz"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "lzzz").first,
                  (11 * (ipow(26, 3) + ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 +
                   25 * (ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 + 25 * (ipow(26, 1) + ipow(26, 0)) + 1 +
                   25 * (ipow(26, 0)) + 1 - bin_2_lower) /
                          bin_2_width * bin_2_count +
                      bin_1_count);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, "mnoo"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "mnoo").first,
                  (bin_2_width - 2) / bin_2_width * bin_2_count + bin_1_count);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, "mnop"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "mnop").first,
                  (bin_2_width - 1) / bin_2_width * bin_2_count + bin_1_count);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, "mnoq"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "mnoq").first, bin_1_count + bin_2_count);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, "oopp"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "oopp").first, bin_1_count + bin_2_count);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, "oopq"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "oopq").first,
                  1 / bin_3_width * bin_3_count + bin_1_count + bin_2_count);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, "oopr"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "oopr").first,
                  2 / bin_3_width * bin_3_count + bin_1_count + bin_2_count);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, "pppp"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "pppp").first,
                  (15 * (ipow(26, 3) + ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 +
                   15 * (ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 + 15 * (ipow(26, 1) + ipow(26, 0)) + 1 +
                   15 * (ipow(26, 0)) + 1 - bin_3_lower) /
                          bin_3_width * bin_3_count +
                      bin_1_count + bin_2_count);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, "qqqq"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "qqqq").first,
                  (16 * (ipow(26, 3) + ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 +
                   16 * (ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 + 16 * (ipow(26, 1) + ipow(26, 0)) + 1 +
                   16 * (ipow(26, 0)) + 1 - bin_3_lower) /
                          bin_3_width * bin_3_count +
                      bin_1_count + bin_2_count);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, "qllo"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "qllo").first,
                  (16 * (ipow(26, 3) + ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 +
                   11 * (ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 + 11 * (ipow(26, 1) + ipow(26, 0)) + 1 +
                   14 * (ipow(26, 0)) + 1 - bin_3_lower) /
                          bin_3_width * bin_3_count +
                      bin_1_count + bin_2_count);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, "qrss"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "qrss").first,
                  (bin_3_width - 2) / bin_3_width * bin_3_count + bin_1_count + bin_2_count);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, "qrst"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "qrst").first,
                  (bin_3_width - 1) / bin_3_width * bin_3_count + bin_1_count + bin_2_count);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, "qrsu"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "qrsu").first,
                  bin_1_count + bin_2_count + bin_3_count);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, "uvwx"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "uvwx").first,
                  bin_1_count + bin_2_count + bin_3_count);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, "uvwy"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "uvwy").first,
                  1 / bin_4_width * bin_4_count + bin_1_count + bin_2_count + bin_3_count);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, "uvwz"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "uvwz").first,
                  2 / bin_4_width * bin_4_count + bin_1_count + bin_2_count + bin_3_count);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, "vvvv"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "vvvv").first,
                  (21 * (ipow(26, 3) + ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 +
                   21 * (ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 + 21 * (ipow(26, 1) + ipow(26, 0)) + 1 +
                   21 * (ipow(26, 0)) + 1 - bin_4_lower) /
                          bin_4_width * bin_4_count +
                      bin_1_count + bin_2_count + bin_3_count);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, "xxxx"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "xxxx").first,
                  (23 * (ipow(26, 3) + ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 +
                   23 * (ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 + 23 * (ipow(26, 1) + ipow(26, 0)) + 1 +
                   23 * (ipow(26, 0)) + 1 - bin_4_lower) /
                          bin_4_width * bin_4_count +
                      bin_1_count + bin_2_count + bin_3_count);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, "ycip"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "ycip").first,
                  (24 * (ipow(26, 3) + ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 +
                   2 * (ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 + 8 * (ipow(26, 1) + ipow(26, 0)) + 1 +
                   15 * (ipow(26, 0)) + 1 - bin_4_lower) /
                          bin_4_width * bin_4_count +
                      bin_1_count + bin_2_count + bin_3_count);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, "yyzy"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "yyzy").first,
                  (bin_4_width - 2) / bin_4_width * bin_4_count + bin_1_count + bin_2_count + bin_3_count);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, "yyzz"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "yyzz").first,
                  (bin_4_width - 1) / bin_4_width * bin_4_count + bin_1_count + bin_2_count + bin_3_count);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, "yz"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "yz").first, total_count);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThan, "zzzz"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "zzzz").first, total_count);
}

TEST_F(EqualDistinctCountHistogramTest, StringLikePrefix) {
  auto hist = EqualDistinctCountHistogram<std::string>::from_segment(
      _string3->get_chunk(ChunkID{0})->get_segment(ColumnID{0}), 4u, "abcdefghijklmnopqrstuvwxyz", 4u);

  // First bin: [abcd, efgh], so everything before is prunable.
  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Like, "a"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Like, "a").first, 0.f);

  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Like, "aa%"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Like, "aa%").first, 0.f);

  // Complexity of prefix pattern does not matter for pruning decision.
  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Like, "aa%zz%"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Like, "aa%zz%").first, 0.f);

  // Even though "aa%" is prunable, "a%" is not!
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Like, "a%"));
  // Since there are no values smaller than "abcd", [abcd, azzz] is the range that "a%" covers.
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Like, "a%").first,
                  hist->estimate_cardinality(PredicateCondition::LessThan, "b").first -
                      hist->estimate_cardinality(PredicateCondition::LessThan, "a").first);
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Like, "a%").first,
                  hist->estimate_cardinality(PredicateCondition::LessThan, "b").first -
                      hist->estimate_cardinality(PredicateCondition::LessThan, "abcd").first);

  // No wildcard, no party.
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Like, "abcd"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Like, "abcd").first,
                  hist->estimate_cardinality(PredicateCondition::Equals, "abcd").first);

  // Classic cases for prefix search.
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Like, "ab%"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Like, "ab%").first,
                  hist->estimate_cardinality(PredicateCondition::LessThan, "ac").first -
                      hist->estimate_cardinality(PredicateCondition::LessThan, "ab").first);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Like, "c%"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Like, "c%").first,
                  hist->estimate_cardinality(PredicateCondition::LessThan, "d").first -
                      hist->estimate_cardinality(PredicateCondition::LessThan, "c").first);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Like, "cfoobar%"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Like, "cfoobar%").first,
                  hist->estimate_cardinality(PredicateCondition::LessThan, "cfoobas").first -
                      hist->estimate_cardinality(PredicateCondition::LessThan, "cfoobar").first);

  // Use upper bin boundary as range limit, since there are no other values starting with e in other bins.
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Like, "e%"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Like, "e%").first,
                  hist->estimate_cardinality(PredicateCondition::LessThan, "f").first -
                      hist->estimate_cardinality(PredicateCondition::LessThan, "e").first);
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Like, "e%").first,
                  hist->estimate_cardinality(PredicateCondition::LessThanEquals, "efgh").first -
                      hist->estimate_cardinality(PredicateCondition::LessThan, "e").first);

  // Second bin starts at ijkl, so there is a gap between efgh and ijkl.
  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Like, "f%"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Like, "f%").first, 0.f);

  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Like, "ii%"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Like, "ii%").first, 0.f);

  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Like, "iizzzzzzzz%"));
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::Like, "iizzzzzzzz%").first, 0.f);
}

TEST_F(EqualDistinctCountHistogramTest, IntBetweenPruning) {
  const auto hist = EqualDistinctCountHistogram<int32_t>::from_segment(
      this->_int_float4->get_chunk(ChunkID{0})->get_segment(ColumnID{0}), 2u);

  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Between, AllTypeVariant{50}, AllTypeVariant{60}));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Between, AllTypeVariant{123}, AllTypeVariant{124}));
  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Between, AllTypeVariant{124}, AllTypeVariant{12'344}));
  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Between, AllTypeVariant{12'344}, AllTypeVariant{12'344}));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Between, AllTypeVariant{12'344}, AllTypeVariant{12'345}));
}

TEST_F(EqualDistinctCountHistogramTest, IntBetweenPruningSpecial) {
  const auto hist = EqualDistinctCountHistogram<int32_t>::from_segment(
      this->_int_float4->get_chunk(ChunkID{0})->get_segment(ColumnID{0}), 1u);

  // Make sure that pruning does not do anything stupid with one bin.
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Between, AllTypeVariant{0}, AllTypeVariant{1'000'000}));
}

TEST_F(EqualDistinctCountHistogramTest, StringCommonPrefix) {
  /**
   * The strings in this table are all eight characters long, but we limit the histogram to a prefix length of four.
   * However, all of the strings start with a common prefix ('aaaa').
   * In this test, we make sure that the calculation strips the common prefix within bins and works as expected.
   */
  auto hist = EqualDistinctCountHistogram<std::string>::from_segment(
      _string_with_prefix->get_chunk(ChunkID{0})->get_segment(ColumnID{0}), 3u, "abcdefghijklmnopqrstuvwxyz", 4u);

  // First bin: [aaaaaaaa, aaaaaaaz].
  // Common prefix: 'aaaaaaa'
  // (repr(m) - repr(a)) / (repr(z) - repr(a) + 1) * bin_1_count
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "aaaaaaam").first,
                  (12.f * (ipow(26, 3) + ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 -
                   (0 * (ipow(26, 3) + ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1)) /
                      (25 * (ipow(26, 3) + ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 -
                       (0 * (ipow(26, 3) + ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1) + 1) *
                      4.f);

  // Second bin: [aaaaffff, aaaaffsd].
  // Common prefix: 'aaaaff'
  // (repr(pr) - repr(ff)) / (repr(sd) - repr(ff) + 1) * bin_2_count + bin_1_count
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "aaaaffpr").first,
                  (15.f * (ipow(26, 3) + ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 +
                   17.f * (ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 -
                   (5 * (ipow(26, 3) + ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 +
                    5 * (ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1)) /
                          (18.f * (ipow(26, 3) + ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 +
                           3.f * (ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 -
                           (5 * (ipow(26, 3) + ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 +
                            5 * (ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1) +
                           1) *
                          4.f +
                      4.f);

  // Second bin: [aaaappwp, aaaazzal].
  // Common prefix: 'aaaa'
  // (repr(tttt) - repr(ppwp)) / (repr(zzal) - repr(ppwp) + 1) * bin_3_count + bin_1_count + bin_2_count
  EXPECT_FLOAT_EQ(hist->estimate_cardinality(PredicateCondition::LessThan, "aaaatttt").first,
                  (19.f * (ipow(26, 3) + ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 +
                   19.f * (ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 + 19.f * (ipow(26, 1) + ipow(26, 0)) + 1 +
                   19.f * ipow(26, 0) + 1 -
                   (15.f * (ipow(26, 3) + ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 +
                    15.f * (ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 + 22.f * (ipow(26, 1) + ipow(26, 0)) + 1 +
                    15.f * ipow(26, 0) + 1)) /
                          (25.f * (ipow(26, 3) + ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 +
                           25.f * (ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 + 0.f * (ipow(26, 1) + ipow(26, 0)) +
                           1 + 11.f * ipow(26, 0) + 1 -
                           (15.f * (ipow(26, 3) + ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 +
                            15.f * (ipow(26, 2) + ipow(26, 1) + ipow(26, 0)) + 1 + 22.f * (ipow(26, 1) + ipow(26, 0)) +
                            1 + 15.f * ipow(26, 0) + 1) +
                           1) *
                          3.f +
                      4.f + 4.f);
}

TEST_F(EqualDistinctCountHistogramTest, StringLikeEdgePruning) {
  /**
   * This test makes sure that pruning works even if the next value after the search prefix is part of a bin.
   * In this case, we check "d%", which is handled as the range [d, e).
   * "e" is the lower edge of the bin that appears after the gap in which "d" falls.
   * A similar situation arises for "v%", but "w" should also be in a gap,
   * because the last bin starts with the next value after "w", i.e., "wa".
   * bins: [aa, bums], [e, uuu], [wa, zzz]
   * For more details see AbstractHistogram::can_prune.
   * We test all the other one-letter prefixes as well, because, why not.
   */
  auto hist = EqualDistinctCountHistogram<std::string>::from_segment(
      _string_like_pruning->get_chunk(ChunkID{0})->get_segment(ColumnID{0}), 3u, "abcdefghijklmnopqrstuvwxyz", 4u);

  // Not prunable, because values start with the character.
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Like, "a%"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Like, "b%"));

  // Prunable, because in a gap.
  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Like, "c%"));

  // This is the interesting part.
  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Like, "d%"));

  // Not prunable, because bin range is [e, uuu].
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Like, "e%"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Like, "f%"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Like, "g%"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Like, "h%"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Like, "i%"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Like, "j%"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Like, "k%"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Like, "l%"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Like, "m%"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Like, "n%"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Like, "o%"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Like, "p%"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Like, "q%"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Like, "r%"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Like, "s%"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Like, "t%"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Like, "u%"));

  // The second more interesting test.
  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::Like, "v%"));

  // Not prunable, because bin range is [wa, zzz].
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Like, "w%"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Like, "x%"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Like, "y%"));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::Like, "z%"));
}

TEST_F(EqualDistinctCountHistogramTest, SliceWithPredicate) {
  // clang-format off
  const auto hist = std::make_shared<EqualDistinctCountHistogram<int32_t>>(
          std::vector<int32_t>{1,  30, 60, 80},
          std::vector<int32_t>{25, 50, 75, 100},
          std::vector<HistogramCountType>{40, 30, 20, 10},
          10, 0);
  // clang-format on
  auto new_hist = std::shared_ptr<GenericHistogram<int32_t>>{};

  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::LessThan, 1));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::LessThanEquals, 1));
  EXPECT_FALSE(hist->does_not_contain(PredicateCondition::GreaterThanEquals, 100));
  EXPECT_TRUE(hist->does_not_contain(PredicateCondition::GreaterThan, 100));

  new_hist =
      std::static_pointer_cast<GenericHistogram<int32_t>>(hist->slice_with_predicate(PredicateCondition::Equals, 15));
  // New histogram should have 15 as min and max.
  EXPECT_TRUE(new_hist->does_not_contain(PredicateCondition::LessThan, 15));
  EXPECT_FALSE(new_hist->does_not_contain(PredicateCondition::LessThanEquals, 15));
  EXPECT_FALSE(new_hist->does_not_contain(PredicateCondition::GreaterThanEquals, 15));
  EXPECT_TRUE(new_hist->does_not_contain(PredicateCondition::GreaterThan, 15));
  EXPECT_FLOAT_EQ(new_hist->estimate_cardinality(PredicateCondition::Equals, 15).first, 40.f / 10);

  new_hist = std::static_pointer_cast<GenericHistogram<int32_t>>(
      hist->slice_with_predicate(PredicateCondition::NotEquals, 15));
  EXPECT_TRUE(new_hist->does_not_contain(PredicateCondition::LessThan, 1));
  EXPECT_FALSE(new_hist->does_not_contain(PredicateCondition::LessThanEquals, 1));
  EXPECT_FALSE(new_hist->does_not_contain(PredicateCondition::GreaterThanEquals, 100));
  EXPECT_TRUE(new_hist->does_not_contain(PredicateCondition::GreaterThan, 100));
  EXPECT_FLOAT_EQ(new_hist->estimate_cardinality(PredicateCondition::Equals, 23).first, (40 - 40.f / 10) / 9);

  new_hist = std::static_pointer_cast<GenericHistogram<int32_t>>(
      hist->slice_with_predicate(PredicateCondition::LessThanEquals, 15));
  // New bin should start at same value as before and end at 15.
  EXPECT_TRUE(new_hist->does_not_contain(PredicateCondition::LessThan, 1));
  EXPECT_FALSE(new_hist->does_not_contain(PredicateCondition::LessThanEquals, 1));
  EXPECT_FALSE(new_hist->does_not_contain(PredicateCondition::GreaterThanEquals, 15));
  EXPECT_TRUE(new_hist->does_not_contain(PredicateCondition::GreaterThan, 15));
  EXPECT_FLOAT_EQ(new_hist->estimate_cardinality(PredicateCondition::Equals, 10).first, 24.f / 6);

  new_hist = std::static_pointer_cast<GenericHistogram<int32_t>>(
      hist->slice_with_predicate(PredicateCondition::LessThanEquals, 27));
  // New bin should start at same value as before and end before first gap (because 27 is in that first gap).
  EXPECT_TRUE(new_hist->does_not_contain(PredicateCondition::LessThan, 1));
  EXPECT_FALSE(new_hist->does_not_contain(PredicateCondition::LessThanEquals, 1));
  EXPECT_FALSE(new_hist->does_not_contain(PredicateCondition::GreaterThanEquals, 25));
  EXPECT_TRUE(new_hist->does_not_contain(PredicateCondition::GreaterThan, 25));
  EXPECT_FLOAT_EQ(new_hist->estimate_cardinality(PredicateCondition::Equals, 10).first, 40.f / 10);

  new_hist = std::static_pointer_cast<GenericHistogram<int32_t>>(
      hist->slice_with_predicate(PredicateCondition::GreaterThanEquals, 15));
  // New bin should start at 15 and end at same value as before.
  EXPECT_TRUE(new_hist->does_not_contain(PredicateCondition::LessThan, 15));
  EXPECT_FALSE(new_hist->does_not_contain(PredicateCondition::LessThanEquals, 15));
  EXPECT_FALSE(new_hist->does_not_contain(PredicateCondition::GreaterThanEquals, 100));
  EXPECT_TRUE(new_hist->does_not_contain(PredicateCondition::GreaterThan, 100));
  EXPECT_FLOAT_EQ(new_hist->estimate_cardinality(PredicateCondition::Equals, 18).first, 18.f / 5);

  new_hist = std::static_pointer_cast<GenericHistogram<int32_t>>(
      hist->slice_with_predicate(PredicateCondition::GreaterThanEquals, 27));
  // New bin should start after the first gap (because 27 is in that first gap) and end at same value as before.
  EXPECT_TRUE(new_hist->does_not_contain(PredicateCondition::LessThan, 30));
  EXPECT_FALSE(new_hist->does_not_contain(PredicateCondition::LessThanEquals, 30));
  EXPECT_FALSE(new_hist->does_not_contain(PredicateCondition::GreaterThanEquals, 100));
  EXPECT_TRUE(new_hist->does_not_contain(PredicateCondition::GreaterThan, 100));
  EXPECT_TRUE(new_hist->does_not_contain(PredicateCondition::Between, 51, 59));
  EXPECT_FLOAT_EQ(new_hist->estimate_cardinality(PredicateCondition::Equals, 35).first, 30.f / 10);

  new_hist = std::static_pointer_cast<GenericHistogram<int32_t>>(
      hist->slice_with_predicate(PredicateCondition::Between, 0, 17));
  // New bin should start at same value as before (because 0 is smaller than the min of the histogram) and end at 17.
  EXPECT_TRUE(new_hist->does_not_contain(PredicateCondition::LessThan, 1));
  EXPECT_FALSE(new_hist->does_not_contain(PredicateCondition::LessThanEquals, 1));
  EXPECT_FALSE(new_hist->does_not_contain(PredicateCondition::GreaterThanEquals, 17));
  EXPECT_TRUE(new_hist->does_not_contain(PredicateCondition::GreaterThan, 17));
  EXPECT_FLOAT_EQ(new_hist->estimate_cardinality(PredicateCondition::Equals, 15).first, 40.f / 10);

  new_hist = std::static_pointer_cast<GenericHistogram<int32_t>>(
      hist->slice_with_predicate(PredicateCondition::Between, 15, 77));
  // New bin should start at 15 and end right before the second gap (because 77 is in that gap).
  EXPECT_TRUE(new_hist->does_not_contain(PredicateCondition::LessThan, 15));
  EXPECT_FALSE(new_hist->does_not_contain(PredicateCondition::LessThanEquals, 15));
  EXPECT_TRUE(new_hist->does_not_contain(PredicateCondition::Between, 51, 59));
  EXPECT_FALSE(new_hist->does_not_contain(PredicateCondition::GreaterThanEquals, 75));
  EXPECT_TRUE(new_hist->does_not_contain(PredicateCondition::GreaterThan, 75));
  EXPECT_FLOAT_EQ(new_hist->estimate_cardinality(PredicateCondition::Equals, 18).first, 18.f / 5);
}

}  // namespace opossum
