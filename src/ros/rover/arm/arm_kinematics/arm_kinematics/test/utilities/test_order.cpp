// order_test.cpp
#include <array>
#include <vector>
#include <arm_kinematics/utilities/order.hpp>
#include <arm_kinematics/utilities/reordered.hpp>
#include <gtest/gtest.h>

// If needed, bring types into scope:
using arm_kinematics::span;
using arm_kinematics::Order;
using arm_kinematics::Reordered;

// ---------- Order<true> tests ----------

TEST(OrderFixedSizeTest, ConstructsWithCorrectSize) {
  constexpr std::size_t N = 5;
  Order<true, std::size_t> order(N);

  EXPECT_EQ(order.size(), N);

  // Default-initialised indices are 0
  for (std::size_t i = 0; i < N; ++i) {
    EXPECT_EQ(order[i], 0u);
  }

  // Can assign into map directly
  for (std::size_t i = 0; i < N; ++i) {
    order[i] = N - 1 - i;
  }

  for (std::size_t i = 0; i < N; ++i) {
    EXPECT_EQ(order[i], N - 1 - i);
  }

  // Iteration yields same
  std::size_t idx = 0;
  for (auto v : order) {
    EXPECT_EQ(v, N - 1 - idx);
    ++idx;
  }
  EXPECT_EQ(idx, N);
}

// ---------- Order<false> tests ----------

TEST(OrderDynamicTest, PushBackAndSize) {
  Order<false, std::size_t> order;

  EXPECT_EQ(order.size(), 0u);

  order.push_back(3);
  order.push_back(1);
  order.push_back(4);

  ASSERT_EQ(order.size(), 3u);
  EXPECT_EQ(order[0], 3u);
  EXPECT_EQ(order[1], 1u);
  EXPECT_EQ(order[2], 4u);

  std::vector<std::size_t> collected;
  for (auto v : order) {
    collected.push_back(v);
  }
  ASSERT_EQ(collected.size(), 3u);
  EXPECT_EQ(collected[0], 3u);
  EXPECT_EQ(collected[1], 1u);
  EXPECT_EQ(collected[2], 4u);
}

TEST(OrderDynamicTest, PushBackDeletionsFromZero) {
  Order<false, std::size_t> order;

  // deletions: false means "keep" → indices 0 and 2
  std::array<bool, 4> del_arr{{false, true, false, true}};
  arm_kinematics::span<bool> deletions(del_arr.data(), del_arr.size());

  order.push_back_deletions(deletions);

  ASSERT_EQ(order.size(), 2u);
  EXPECT_EQ(order[0], 0u);
  EXPECT_EQ(order[1], 2u);
}

TEST(OrderDynamicTest, PushBackDeletionsWithOffset) {
  Order<false, std::size_t> order;

  // deletions: keep 0 and 2, but offset by 10 → 10 and 12
  std::array<bool, 4> del_arr{{false, true, false, true}};
  arm_kinematics::span<bool> deletions(del_arr.data(), del_arr.size());

  order.push_back_deletions(10u, deletions);

  ASSERT_EQ(order.size(), 2u);
  EXPECT_EQ(order[0], 10u);
  EXPECT_EQ(order[1], 12u);
}

// ---------- Reordered tests (fixed-size) ----------

TEST(ReorderedFixedSizeTest, IndexingAndIteration) {
  // Underlying data
  std::vector<int> data{10, 20, 30, 40};

  // Fixed-size order: reverse indices
  Order<true, std::size_t> order(data.size());
  for (std::size_t i = 0; i < order.size(); ++i) {
    order[i] = data.size() - 1 - i;
  }

  Reordered<std::vector<int>, std::size_t, true> reordered{data, order};

  // operator[]
  EXPECT_EQ(reordered[0], 40);
  EXPECT_EQ(reordered[1], 30);
  EXPECT_EQ(reordered[2], 20);
  EXPECT_EQ(reordered[3], 10);

  // iteration
  std::vector<int> seen;
  for (auto &v : reordered) {
    seen.push_back(v);
  }

  ASSERT_EQ(seen.size(), 4u);
  EXPECT_EQ(seen[0], 40);
  EXPECT_EQ(seen[1], 30);
  EXPECT_EQ(seen[2], 20);
  EXPECT_EQ(seen[3], 10);
}

// ---------- Reordered tests (dynamic) ----------

TEST(ReorderedDynamicTest, IndexingAndIteration) {
  std::vector<int> data{5, 6, 7, 8, 9};

  Order<false, std::size_t> order;
  order.push_back(1); // -> 6
  order.push_back(3); // -> 8
  order.push_back(4); // -> 9

  Reordered<std::vector<int>, std::size_t, false> reordered{data, order};

  // operator[]
  ASSERT_EQ(order.size(), 3u);
  EXPECT_EQ(reordered[0], 6);
  EXPECT_EQ(reordered[1], 8);
  EXPECT_EQ(reordered[2], 9);

  // iteration
  std::vector<int> seen;
  for (auto &v : reordered) {
    seen.push_back(v);
  }

  ASSERT_EQ(seen.size(), 3u);
  EXPECT_EQ(seen[0], 6);
  EXPECT_EQ(seen[1], 8);
  EXPECT_EQ(seen[2], 9);
}
