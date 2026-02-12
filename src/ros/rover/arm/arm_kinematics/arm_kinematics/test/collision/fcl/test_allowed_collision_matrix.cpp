
#include <gtest/gtest.h>
#include "arm_kinematics/collision/allowed_collision_matrix.hpp"
#include <algorithm>

using arm_kinematics::AllowedCollisionMatrix;

class AllowedCollisionMatrixTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
  }

  void TearDown() override
  {
  }
};

namespace {

// Utility to size the bitset for N
static inline size_t PackedWordCount(uint32_t N) {
  const uint64_t pairs = (static_cast<uint64_t>(N) * (N - 1)) / 2;
  return static_cast<size_t>((pairs + 63) / 64);
}

}

TEST_F(AllowedCollisionMatrixTest, SelfPairsAlwaysAllowed) {
  const uint32_t N = 5;
  AllowedCollisionMatrix acm(N);
  acm.acm_bits.assign(PackedWordCount(N), 0);

  for (uint32_t i = 0; i < N; ++i) {
    EXPECT_TRUE(acm.get(i, i)) << "Self pair should be allowed";
  }
}

TEST_F(AllowedCollisionMatrixTest, DefaultsDisallowAllNonDiagonal) {
  const uint32_t N = 6;
  AllowedCollisionMatrix acm(N);
  acm.acm_bits.assign(PackedWordCount(N), 0);

  for (uint32_t a = 0; a < N; ++a) {
    for (uint32_t b = 0; b < N; ++b) {
      if (a == b) continue;
      EXPECT_FALSE(acm.get(a, b)) << a << "," << b;
    }
  }
}

TEST_F(AllowedCollisionMatrixTest, SetAndUnsetSymmetric) {
  const uint32_t N = 8;
  AllowedCollisionMatrix acm(N);
  acm.acm_bits.assign(PackedWordCount(N), 0);

  // Set a few pairs
  acm.set(1, 5, true);
  acm.set(0, 7, true);
  acm.set(3, 4, true);

  EXPECT_TRUE(acm.get(1, 5));
  EXPECT_TRUE(acm.get(5, 1));

  EXPECT_TRUE(acm.get(0, 7));
  EXPECT_TRUE(acm.get(7, 0));

  EXPECT_TRUE(acm.get(3, 4));
  EXPECT_TRUE(acm.get(4, 3));

  // Unset one
  acm.set(0, 7, false);
  EXPECT_FALSE(acm.get(0, 7));
  EXPECT_FALSE(acm.get(7, 0));

  // Others unaffected
  EXPECT_TRUE(acm.get(1, 5));
  EXPECT_TRUE(acm.get(3, 4));
}

TEST_F(AllowedCollisionMatrixTest, TriIndexCoversAllPairsWithoutOverlap) {
  const uint32_t N = 10;
  AllowedCollisionMatrix acm(N);
  acm.acm_bits.assign(PackedWordCount(N), 0);

  const uint64_t pairs = (static_cast<uint64_t>(N) * (N - 1)) / 2;
  std::vector<uint64_t> indices;
  indices.reserve(pairs);

  for (uint32_t a = 0; a < N; ++a) {
    for (uint32_t b = a + 1; b < N; ++b) {
      indices.push_back(acm.tri_index(a, b));
    }
  }

  std::sort(indices.begin(), indices.end());
  // Must be 0..pairs-1 in some order
  EXPECT_EQ(indices.front(), 0ULL);
  EXPECT_EQ(indices.back(), pairs - 1);
  // No duplicates
  EXPECT_EQ(std::adjacent_find(indices.begin(), indices.end()), indices.end());
  // All in-range
  for (auto i : indices) EXPECT_LT(i, pairs);
}

TEST_F(AllowedCollisionMatrixTest, BitStorageMatchesPairCount) {
  for (uint32_t N : {0u, 1u, 2u, 3u, 16u, 255u}) {
    AllowedCollisionMatrix acm(N);
    acm.acm_bits.assign(PackedWordCount(N), 0);
    const uint64_t pairs = (static_cast<uint64_t>(N) * (N - 1)) / 2;
    const uint64_t bits  = static_cast<uint64_t>(acm.acm_bits.size()) * 64ULL;

    EXPECT_GE(bits, pairs);

    // Flip every pair to ensure we never write out of bounds
    for (uint32_t a = 0; a < N; ++a) {
      for (uint32_t b = a + 1; b < N; ++b) {
        acm.set(a, b, true);
      }
    }

    // Spot check a few
    if (N >= 3) {
      EXPECT_TRUE(acm.get(0, 1));
      EXPECT_TRUE(acm.get(0, 2));
      EXPECT_TRUE(acm.get(1, 2));
    }
  }
}

TEST_F(AllowedCollisionMatrixTest, SymmetryAndDiagonalDoNotAlias) {
  const uint32_t N = 12;
  AllowedCollisionMatrix acm(N);
  acm.acm_bits.assign(PackedWordCount(N), 0);

  // Set a random subset
  acm.set(2, 11, true);
  acm.set(5, 6,  true);
  acm.set(0, 9,  true);

  // Symmetry
  EXPECT_TRUE(acm.get(11, 2));
  EXPECT_TRUE(acm.get(6, 5));
  EXPECT_TRUE(acm.get(9, 0));

  // Diagonal always allowed and does not interact with bits
  for (uint32_t i = 0; i < N; ++i)
    EXPECT_TRUE(acm.get(i, i));
}

