//
// Created by Bailey Chessum on 12/11/2025.
//

#ifndef ARM_KINEMATICS_ALLOWED_COLLISION_MATRIX_HPP
#define ARM_KINEMATICS_ALLOWED_COLLISION_MATRIX_HPP

#include <cstdint>
#include <vector>

namespace arm_kinematics {

/**
 * Defines collision object pairs that are allowed to intersect, implemented as a packed triangular matrix.
 *
 * \note Must know the max number of elements before constructing. Space complexity O(N^2).
 */
struct AllowedCollisionMatrix {
  /// Allowed Collision Matrix as a packed triangular bitset
  std::vector<uint64_t> acm_bits;

  /// Number of collision objects
  uint64_t capacity = 0;

  /**
   * Constructor -- Specify the number of collision elements here, then set each allowed collision before using
   * \param capacity The number of collision objects, which determines the max size of the matrix.
   */
  explicit AllowedCollisionMatrix(uint64_t capacity);

  /**
   * Makes a copy of a given ACM but with a different capacity.
   * \param capacity The number of collision objects, which determines the max size of the matrix.
   */
  explicit AllowedCollisionMatrix(const AllowedCollisionMatrix& other, uint64_t capacity);

  AllowedCollisionMatrix() : AllowedCollisionMatrix(0) {}

  AllowedCollisionMatrix(AllowedCollisionMatrix&& other) noexcept;

  inline void resize(uint64_t new_size) {
    auto temp = AllowedCollisionMatrix(*this, new_size);
    acm_bits = std::move(temp.acm_bits);
    capacity = temp.capacity;
  }

  inline void reserve(uint64_t new_capacity) {
    if (capacity >= new_capacity)
      return;

    resize(new_capacity);
  }

  /**
   * Checks if two colliders are allowed to collide.
   * \note This can't be implemented as a custom indexer without introducing proxies.
   * \param a obj-id of the first collider
   * \param b obj-id of the second collider
   * \returns True if collision between a and b should be ignored
   */
  [[nodiscard]] inline bool get(uint32_t a, uint32_t b) const {
    if (a == b)
      return true;

    uint64_t i = tri_index(a, b);
    return (acm_bits[i >> 6] >> (i & 63)) & 1ULL;
  }

  /**
   * Sets if two colliders are allowed to collide
   * \param a obj-id of the first collider
   * \param b obj-id of the second collider
   */
  inline void set(uint32_t a, uint32_t b, bool allowed) {
    if (a == b)
      return;

    // Amortized linear? Try to avoid :)
    if (b >= capacity || a >= capacity) {
      resize(std::max(capacity, 1UL) * 2);
    }


    uint64_t i = tri_index(a, b);
    uint64_t m = 1ULL << (i & 63);

    if (allowed)
      acm_bits[i >> 6] |= m;
    else
      acm_bits[i >> 6] &= ~m;
  }

  /**
   * Gets the triangular index for some pair of object ids a and b, where a < b.
   */
  [[nodiscard]] inline uint64_t tri_index(uint32_t a, uint32_t b) const {
    if (a > b)
      std::swap(a, b);

    // base for row a in upper triangle without diagonal
    uint64_t a64 = a;
    uint64_t N64 = capacity;
    uint64_t base = a64 * N64 - (a64 * (a64 + 1)) / 2;
    return base + (b - a - 1);
  }
};

} // arm_kinematics

#endif //ARM_KINEMATICS_ALLOWED_COLLISION_MATRIX_HPP
