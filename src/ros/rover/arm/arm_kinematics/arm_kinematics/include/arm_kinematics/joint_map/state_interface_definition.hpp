//
// Created by Bailey Chessum on 5/4/26.
//

#ifndef ARM_KINEMATICS_STATE_INTERFACE_DEFINITION_HPP
#define ARM_KINEMATICS_STATE_INTERFACE_DEFINITION_HPP

#include <functional>
#include <string>
#include <utility>

#include "arm_kinematics/joint_map/transmission_types.hpp"
#include "arm_kinematics/utilities/interface_id.hpp"

namespace arm_kinematics {

struct NamedStateInterfaceDefinition {
  std::string joint_name;
  InterfaceId interface_id{};

  explicit NamedStateInterfaceDefinition(std::string name)
    : joint_name(std::move(name))
  {
  }

  NamedStateInterfaceDefinition(std::string name, InterfaceId interface_id)
    : joint_name(std::move(name)), interface_id(std::move(interface_id))
  {
  }

  [[nodiscard]] std::string format() const
  {
    return joint_name + "/" + interface_id.name;
  }

  bool operator==(const NamedStateInterfaceDefinition & other) const noexcept
  {
    return joint_name == other.joint_name && interface_id == other.interface_id;
  }

  bool operator!=(const NamedStateInterfaceDefinition & other) const noexcept
  {
    return !(*this == other);
  }
};

struct StateInterfaceDefinition {
  JointId joint_id = 0;
  InterfaceId interface_id{};

  StateInterfaceDefinition() = default;
  StateInterfaceDefinition(const JointId id, InterfaceId interface_id)
    : joint_id(id), interface_id(std::move(interface_id))
  {
  }

  [[nodiscard]] std::string format() const
  {
    return "joint_id=" + std::to_string(joint_id) + "/" + interface_id.name;
  }

  bool operator==(const StateInterfaceDefinition & other) const noexcept
  {
    // Delegate to InterfaceId::operator==, which checks hash first (fast) and then name
    // (collision-safe). Comparing only the hash here would be unsound — two distinct interface
    // names could in principle collide on FNV1a-64 (vanishingly unlikely, but still incorrect).
    return joint_id == other.joint_id && interface_id == other.interface_id;
  }

  bool operator!=(const StateInterfaceDefinition & other) const noexcept
  {
    return !(*this == other);
  }
};

} // namespace arm_kinematics

namespace std {

template <>
struct hash<arm_kinematics::StateInterfaceDefinition> {
  std::size_t operator()(const arm_kinematics::StateInterfaceDefinition & value) const noexcept
  {
    std::size_t seed = std::hash<arm_kinematics::JointId>{}(value.joint_id);
    const std::size_t hash_value = value.interface_id.hash;
    seed ^= hash_value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
    return seed;
  }
};

} // namespace std

#endif //ARM_KINEMATICS_STATE_INTERFACE_DEFINITION_HPP
