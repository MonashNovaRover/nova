//
// Created by Bailey Chessum on 12/11/2025.
//

#include <arm_kinematics/collision/allowed_collision_matrix.hpp>

namespace arm_kinematics {

static constexpr inline size_t packed_word_count(const std::size_t N) {
  const uint64_t pairs = (static_cast<uint64_t>(N) * (N - 1)) / 2;
  return static_cast<size_t>((pairs + 63) / 64);
}

AllowedCollisionMatrix::AllowedCollisionMatrix(const std::size_t capacity) : capacity(capacity) {
  acm_bits.resize(packed_word_count(capacity), 0);
}

AllowedCollisionMatrix::AllowedCollisionMatrix(const AllowedCollisionMatrix & other, const std::size_t capacity) : capacity(capacity) {
  // Only allow for growing, not shrinking
  if (capacity < other.capacity) {
    this->capacity = other.capacity;
    acm_bits = other.acm_bits;
    return;
  }

  acm_bits.resize(packed_word_count(capacity), 0);

  // Copy all values from the old matrix
  for (size_t b = 0; b < other.capacity; b++) {
    for (size_t a = 0; a < b; b++) {
      set(a, b, other.get(a, b));
    }
  }
}

} // arm_kinematics