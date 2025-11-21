//
// Created by Bailey Chessum on 17/11/2025.
//

#include <arm_kinematics/forward/eigen/eigen_forward_kinematics_plugin.hpp>

namespace arm_kinematics {

static_assert(std::is_base_of_v<
  ForwardKinematicsPlugin::Tree,
  EigenForwardKinematicsPlugin::TreeImpl>);

ForwardKinematicsPlugin::Tree::SharedPtr EigenForwardKinematicsPlugin::make_tree(
  const std::vector<std::string> & joint_names, const std::string & base_link_name, FrameDefinitions frames,
  const JointMapBuilder & joint_map_builder) {
  std::vector<std::string> mapper_joint_names{};
  const size_t output_count = frames.origins.size();

  ComputeFrameTree mapper = build_fk_mapper_from_urdf(
    get_urdf_model(),
    base_link_name,
    std::move(frames), //< changed method call signature to remove const and &
    mapper_joint_names);

  std::shared_ptr<TreeImpl> ptr = std::make_shared<TreeImpl>(
    output_count,
    mapper,
    joint_map_builder.build(joint_names, mapper_joint_names));

  return ptr;
}

} // arm_kinematics