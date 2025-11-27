//
// Created by Bailey Chessum on 17/11/2025.
//

#include <arm_kinematics/forward/eigen/eigen_forward_kinematics_plugin.hpp>
#include <arm_kinematics/forward/eigen/compute_frame_tree.hpp>

namespace arm_kinematics {

static_assert(std::is_base_of_v<
  ForwardKinematicsPlugin::Tree,
  EigenForwardKinematicsPlugin::TreeImpl>);

ForwardKinematicsPlugin::MakeTreeResult EigenForwardKinematicsPlugin::make_tree(
  const std::vector<std::string> & joint_names,
  const std::string & base_link_name,
  const FrameDefinitions & frames,
  const JointMapBuilder & joint_map_builder)
{
  AnalysisTree subtree(tree_, base_link_name, frames);

  subtree.log(get_logger());

  // Joints need to be in the right order to be able to construct a compute frame tree
  subtree.sort_joints();
  const std::vector<std::string> & mapper_joint_names = {subtree.get_joints().names.begin() + 1, subtree.get_joints().names.end()};

  subtree.log(get_logger());

  // Sort frames such that any root relative frames are placed at the end of the array
  auto frame_order = std::make_unique<Order<>>(subtree.sort_frames());

  // Create the compute tree
  auto compute_frame_tree = subtree.make_compute_frame_tree();
  if (!compute_frame_tree.has_value()) {
    RCLCPP_ERROR(get_logger(), "Failed to make FK Tree: %s", compute_frame_tree.error().data());
    compute_frame_tree = ComputeFrameTree();  //< Use an empty tree that will do nothing
  }

  auto ptr = std::make_shared<TreeImpl>(
    frames.size(),
    std::move(compute_frame_tree.value()),
    joint_map_builder.build(joint_names, mapper_joint_names));

  return MakeTreeResult{
    std::move(ptr),
    std::move(frame_order)
  };
}

bool EigenForwardKinematicsPlugin::on_initialize() {
  // TODO: Could we precompute joints we wouldn't have the values for here?

  tree_ = AnalysisTree(get_urdf_model());
  tree_.log(get_logger());

  return true;
}

} // arm_kinematics