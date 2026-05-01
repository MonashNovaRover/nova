//
// Created by Bailey Chessum on 21/03/2026.
//

// JointMapBuilder is a pure virtual interface — no out-of-line definitions are needed in the
// base class. Concrete implementations (DefaultJointMapBuilder, FK-plugin-specific builders)
// provide the build_expected body. The legacy `build()` compatibility wrapper that threw on
// JointMapBuildError has been removed; callers use build_expected directly and pattern-match on
// tl::expected.

#include "arm_kinematics/joint_map/joint_map_builder.hpp"

#include <algorithm>
#include <sstream>
#include <variant>

namespace arm_kinematics {

namespace {

constexpr std::size_t kMaxFormattedInterfaces = 50;

template <typename T>
std::string format_state_interface_list(const std::vector<T> & defs)
{
  std::ostringstream oss;
  oss << "[";
  const std::size_t shown = std::min(defs.size(), kMaxFormattedInterfaces);
  for (std::size_t i = 0; i < shown; ++i) {
    if (i > 0) {
      oss << ", ";
    }
    oss << defs[i].format();
  }
  if (defs.size() > shown) {
    oss << ", ...and " << (defs.size() - shown) << " more";
  }
  oss << "]";
  return oss.str();
}

std::vector<StateInterfaceDefinition> ambiguous_defs(
  const std::vector<producers::AmbiguousInterface> & ambiguous_interfaces)
{
  std::vector<StateInterfaceDefinition> defs;
  defs.reserve(ambiguous_interfaces.size());
  for (const auto & ambiguous : ambiguous_interfaces) {
    defs.push_back(ambiguous.interface);
  }
  return defs;
}

}  // namespace

std::string JointMapBuildError::MissingInputs::format() const
{
  return "DefaultJointMapBuilder: " + std::to_string(unproducible_outputs.size()) +
         " requested output(s) are not derivable from the supplied inputs: " +
         format_state_interface_list(unproducible_outputs);
}

std::string JointMapBuildError::Ambiguous::format() const
{
  const auto defs = ambiguous_defs(ambiguous_interfaces);
  return "DefaultJointMapBuilder: " + std::to_string(ambiguous_interfaces.size()) +
         " ambiguous interface(s) block the request: " + format_state_interface_list(defs);
}

std::string JointMapBuildError::UnknownJoint::format() const
{
  std::ostringstream oss;
  oss << "DefaultJointMapBuilder: " << unknown_joints.size()
      << " joint name(s) in the request are not registered in the analysis: [";
  const std::size_t shown = std::min(unknown_joints.size(), kMaxFormattedInterfaces);
  for (std::size_t i = 0; i < shown; ++i) {
    if (i > 0) {
      oss << ", ";
    }
    oss << unknown_joints[i];
  }
  if (unknown_joints.size() > shown) {
    oss << ", ...and " << (unknown_joints.size() - shown) << " more";
  }
  oss << "]";
  return oss.str();
}

std::string JointMapBuildError::format() const
{
  return std::visit([](const auto & error) { return error.format(); }, value);
}

}  // namespace arm_kinematics
