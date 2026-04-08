//
// Created by Bailey Chessum on 6/4/26.
//

#ifndef ARM_KINEMATICS_INTERFACE_ID_HPP
#define ARM_KINEMATICS_INTERFACE_ID_HPP

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <utility>

#include "arm_kinematics/utilities/hash_interface.hpp"

namespace arm_kinematics {

/**
 * Identifier for a state interface type (e.g. "position", "velocity", "effort").
 *
 * Stores both the human-readable name and a precomputed FNV1a hash. The hash is used
 * for fast equality and unordered-map keying; the name is preserved so that error
 * messages and debug output can refer to the interface by its real name rather than
 * an opaque integer.
 *
 * String literal constructors are constexpr-friendly via hash_interface(), so
 * `InterfaceId{"position"}` produces a stable hash at compile time even though the
 * std::string member must be initialized at runtime.
 */
struct InterfaceId {
  std::string name;
  std::size_t hash;

  InterfaceId() noexcept
    : name{}, hash{hash_interface(std::string_view{})}
  {
  }

  template <std::size_t N>
  InterfaceId(const char (&literal)[N])
    : name{literal, N - 1}, hash{hash_interface(literal)}
  {
  }

  InterfaceId(std::string_view value)
    : name{value}, hash{hash_interface(value)}
  {
  }

  InterfaceId(std::string value)
    : name{std::move(value)}, hash{hash_interface(name)}
  {
  }

  bool operator==(const InterfaceId & other) const noexcept
  {
    // Hash check first for speed; name compare for collision safety.
    return hash == other.hash && name == other.name;
  }

  bool operator!=(const InterfaceId & other) const noexcept
  {
    return !(*this == other);
  }
};

} // namespace arm_kinematics

namespace std {

template <>
struct hash<arm_kinematics::InterfaceId> {
  std::size_t operator()(const arm_kinematics::InterfaceId & value) const noexcept
  {
    return value.hash;
  }
};

} // namespace std

#endif // ARM_KINEMATICS_INTERFACE_ID_HPP
