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
  std::vector<std::uint64_t> acm_bits;

  /// Number of collision objects
  std::size_t capacity = 0;

  /**
   * Constructor -- Specify the number of collision elements here, then set each allowed collision before using
   * \param capacity The number of collision objects, which determines the max size of the matrix.
   */
  explicit AllowedCollisionMatrix(uint64_t capacity);

  /**
   * Makes a copy of a given ACM but with a different capacity.
   * \param other ACM to copy values from
   * \param capacity The number of collision objects, which determines the max size of the matrix.
   */
  explicit AllowedCollisionMatrix(const AllowedCollisionMatrix& other, std::size_t capacity);

  AllowedCollisionMatrix() : AllowedCollisionMatrix(0) {}
  AllowedCollisionMatrix(AllowedCollisionMatrix&& other) noexcept;

  inline void resize(std::size_t new_size) {
    auto temp = AllowedCollisionMatrix(*this, new_size);
    acm_bits = std::move(temp.acm_bits);
    capacity = temp.capacity;
  }

  inline void reserve(std::size_t new_capacity) {
    if (capacity >= new_capacity)
      return;
    resize(new_capacity);
  }

  [[nodiscard]] inline bool get(std::size_t a, std::size_t b) const {
    if (a == b)
      return true;

    std::size_t i = tri_index(a, b);
    return (acm_bits[i >> 6] >> (i & 63)) & 1ULL;
  }

  inline void set(std::size_t a, std::size_t b, bool allowed) {
    if (a == b)
      return;

    if (b >= capacity || a >= capacity) {
      resize(std::max<std::size_t>(capacity ? capacity * 2 : 1, std::max(a, b) + 1));
    }

    std::size_t i = tri_index(a, b);
    std::uint64_t m = 1ULL << (i & 63);

    if (allowed)
      acm_bits[i >> 6] |= m;
    else
      acm_bits[i >> 6] &= ~m;
  }

private:
  [[nodiscard]] inline std::size_t tri_index(std::size_t a, std::size_t b) const {
    if (a > b)
      std::swap(a, b);

    // base for row a in upper triangle without diagonal
    std::size_t N = capacity;
    std::size_t base = a * N - (a * (a + 1)) / 2;
    return base + (b - a - 1);
  }
};

} // arm_kinematics

#endif //ARM_KINEMATICS_ALLOWED_COLLISION_MATRIX_HPP
