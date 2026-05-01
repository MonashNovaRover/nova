//
// Created by Bailey Chessum on 6/4/26.
//

#ifndef ARM_KINEMATICS_HASH_LITERAL_HPP
#define ARM_KINEMATICS_HASH_LITERAL_HPP

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace arm_kinematics {

template <size_t N>
constexpr std::size_t hash_interface(const char (&literal)[N]) noexcept
{
  // C++17 lacks a constexpr standard hash for string literals; this provides a stable compile-time ID.
  std::uint64_t hash = 14695981039346656037ull;
  for (size_t i = 0; i + 1 < N; ++i) {
    hash ^= static_cast<const unsigned char>(literal[i]);
    hash *= 1099511628211ull;
  }
  return hash;
}

inline std::size_t hash_interface(const std::string_view value) noexcept
{
  std::uint64_t hash = 14695981039346656037ull;
  for (const unsigned char ch : value) {
    hash ^= ch;
    hash *= 1099511628211ull;
  }
  return hash;
}

} // namespace arm_kinematics

#endif // ARM_KINEMATICS_HASH_LITERAL_HPP
