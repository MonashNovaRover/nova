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
  explicit AllowedCollisionMatrix(std::size_t capacity);

  /**
   * Makes a copy of a given ACM but with a different capacity.
   * \param other ACM to copy values from
   * \param capacity The number of collision objects, which determines the max size of the matrix.
   */
  explicit AllowedCollisionMatrix(const AllowedCollisionMatrix& other, std::size_t capacity);

  AllowedCollisionMatrix() : AllowedCollisionMatrix(0) {}
  AllowedCollisionMatrix(AllowedCollisionMatrix&& other) noexcept
    : acm_bits(std::move(other.acm_bits)), capacity(other.capacity)
  {
    other.capacity = 0;
  }

  AllowedCollisionMatrix & operator=(AllowedCollisionMatrix && other) noexcept {
    // A self move causes acm_bits to become empty. This guards against that, despite being annoying.
    if (this == &other)
      return *this;

    acm_bits = std::move(other.acm_bits);
    capacity = other.capacity;
    other.capacity = 0;
    return *this;
  }

  void resize(const std::size_t new_size) {
    auto temp = AllowedCollisionMatrix(*this, new_size);
    acm_bits = std::move(temp.acm_bits);
    capacity = temp.capacity;
  }

  void reserve(const std::size_t new_capacity) {
    if (capacity >= new_capacity)
      return;
    resize(new_capacity);
  }

  [[nodiscard]] bool get(const std::size_t a, const std::size_t b) const {
    if (a == b)
      return true;

    const std::size_t i = tri_index(a, b);
    return (acm_bits[i >> 6] >> (i & 63)) & 1ULL;
  }

  void set(const std::size_t a, const std::size_t b, const bool allowed) {
    if (a == b)
      return;

    if (b >= capacity || a >= capacity) {
      resize(std::max<std::size_t>(capacity ? capacity * 2 : 1, std::max(a, b) + 1));
    }

    const std::size_t i = tri_index(a, b);
    const std::uint64_t m = 1ULL << (i & 63);

    if (allowed)
      acm_bits[i >> 6] |= m;
    else
      acm_bits[i >> 6] &= ~m;
  }

  [[nodiscard]] inline std::size_t tri_index(std::size_t a, std::size_t b) const {
    if (a > b)
      std::swap(a, b);

    // base for row a in upper triangle without diagonal
    const std::size_t N = capacity;
    const std::size_t base = a * N - (a * (a + 1)) / 2;
    return base + (b - a - 1);
  }
};

} // arm_kinematics

#endif //ARM_KINEMATICS_ALLOWED_COLLISION_MATRIX_HPP
