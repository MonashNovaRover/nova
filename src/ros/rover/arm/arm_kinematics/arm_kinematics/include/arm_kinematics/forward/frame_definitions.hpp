//
// Created by nova on 11/15/25.
//

#ifndef SCIENCE_FRAME_DEFINITIONS_HPP
#define SCIENCE_FRAME_DEFINITIONS_HPP

#include <arm_kinematics/utilities/aliases.hpp>
#include <arm_kinematics/visibility_control.h>
#include <Eigen/Geometry>

namespace arm_kinematics {

/**
 * Struct to help request some pose relative to a link in FK
 */
struct ARM_KINEMATICS_PUBLIC FrameDefinitions {
  Isometry3dVector origins;
  std::vector<std::string> parent_link_names;

  FrameDefinitions(std::vector<std::string> parent_link_names, Isometry3dVector origins)
    : origins(std::move(origins)), parent_link_names(std::move(parent_link_names))
  {
    assert(parent_link_names.size() == origins.size());
  }
  FrameDefinitions(const std::string & parent_link_name, Eigen::Isometry3d origin)
    : FrameDefinitions(std::vector{parent_link_name}, Isometry3dVector{std::move(origin)}) {}

  // Constructor overloads that use identity origins
  FrameDefinitions(std::vector<std::string> link_names)
    : FrameDefinitions(std::move(link_names), Isometry3dVector(link_names.size(), Eigen::Isometry3d::Identity())) {}
  FrameDefinitions(const std::string & link_name)
    : FrameDefinitions(std::vector{link_name}, {Eigen::Isometry3d::Identity()}) {}

  [[nodiscard]] size_t size() const noexcept {
    assert(parent_link_names.size() == origins.size());
    return parent_link_names.size();
  }
};

} // namespace arm_kinematics

#endif //SCIENCE_FRAME_DEFINITIONS_HPP