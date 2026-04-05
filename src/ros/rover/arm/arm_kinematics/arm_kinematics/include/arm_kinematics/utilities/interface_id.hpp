//
// Created by Bailey Chessum on 6/4/26.
//

#ifndef ARM_KINEMATICS_INTERFACE_ID_HPP
#define ARM_KINEMATICS_INTERFACE_ID_HPP

#include <cstddef>
#include <string_view>

#include "arm_kinematics/utilities/hash_interface.hpp"

namespace arm_kinematics {

using InterfaceId = std::string;

// struct InterfaceId {
//   std::size_t hash;
//
//   constexpr InterfaceId() noexcept
//     : hash(hash_interface("position"))
//   {
//   }
//
//   template <size_t N>
//   constexpr InterfaceId(const char (&literal)[N]) noexcept
//     : hash(hash_interface(literal))
//   {
//   }
//
//   InterfaceId(std::string_view value) noexcept
//     : hash(hash_interface(value))
//   {
//   }
// };

} // namespace arm_kinematics

#endif // ARM_KINEMATICS_INTERFACE_ID_HPP
