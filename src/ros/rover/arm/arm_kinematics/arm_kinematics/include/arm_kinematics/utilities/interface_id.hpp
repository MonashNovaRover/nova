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
 * Stores both a precomputed FNV1a hash and the human-readable name. The hash is used
 * for fast equality and unordered-map keying; the name is preserved so that error
 * messages and debug output can refer to the interface by its real name rather than
 * an opaque integer.
 *
 * \note Field order is `hash` then `name` deliberately, so that constructors taking
 * a `std::string` value can compute the hash from the source argument before moving
 * it into `name` — order-independent semantics regardless of how the initializer list
 * is written. (C++ initializes fields in declaration order, not init-list order.)
 */
struct InterfaceId {
  std::size_t hash;

  // TODO: Turn into hash only, with name resolved externally for error reporting
  std::string name;

  InterfaceId() noexcept
    : hash{hash_interface(std::string_view{})}, name{}
  {
  }

  template <std::size_t N>
  InterfaceId(const char (&literal)[N])
    : hash{hash_interface(literal)}, name{literal, N - 1}
  {
  }

  InterfaceId(std::string_view value)
    : hash{hash_interface(value)}, name{value}
  {
  }

  InterfaceId(std::string value)
    // Compute hash from `value` (still valid), then move it into `name`. Order-safe
    // because `hash` is declared first, so it's initialized first regardless of how
    // we list the initializers below.
    : hash{hash_interface(value)}, name{std::move(value)}
  {
  }

  [[nodiscard]] static const InterfaceId & Position()
  {
    static const InterfaceId value{"position"};
    return value;
  }

  [[nodiscard]] static const InterfaceId & Velocity()
  {
    static const InterfaceId value{"velocity"};
    return value;
  }

  [[nodiscard]] static const InterfaceId & Acceleration()
  {
    static const InterfaceId value{"acceleration"};
    return value;
  }

  [[nodiscard]] static const InterfaceId & Effort()
  {
    static const InterfaceId value{"effort"};
    return value;
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
