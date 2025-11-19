//
// Created by nova on 11/15/25.
//

#ifndef SCIENCE_FRAME_DEFINITIONS_HPP
#define SCIENCE_FRAME_DEFINITIONS_HPP

#include <arm_kinematics/aliases.hpp>
#include <Eigen/Geometry>

namespace arm_kinematics {

/**
 * Struct to help request some pose relative to a link in FK
 */
struct FrameDefinitions {
  Isometry3dVector origins;
  std::vector<std::string> parent_link_names;

  FrameDefinitions(std::vector<std::string> parent_link_names, Isometry3dVector origins)
    : origins(std::move(origins)), parent_link_names(std::move(parent_link_names))
  {
    assert(parent_link_names.size() == origins.size());
  }
  FrameDefinitions(std::string parent_link_name, Eigen::Isometry3d origin)
    : FrameDefinitions(std::vector{std::move(parent_link_name)}, {std::move(origin)}) {}

  // Constructor overloads that use identity origins
  FrameDefinitions(std::vector<std::string> link_names)
    : FrameDefinitions(std::move(link_names), Isometry3dVector(link_names.size(), Eigen::Isometry3d::Identity())) {}
  FrameDefinitions(std::string link_name)
    : FrameDefinitions({std::move(link_name)}, {Eigen::Isometry3d::Identity()}) {}

  [[nodiscard]] constexpr size_t size() const noexcept {
    assert(parent_link_names.size() == origins.size());
    return parent_link_names.size();
  }
};

} // namespace arm_kinematics

#endif //SCIENCE_FRAME_DEFINITIONS_HPP