//
// Created by Bailey Chessum on 15/12/2025.
//

#include "arm_kinematics/common/robot_model.hpp"

namespace arm_kinematics {

RobotModel::RobotModel(std::string robot_description)
: robot_description_(std::move(robot_description))
{
}

const std::string & RobotModel::get_robot_description() const {
  return robot_description_;
}

const urdf::Model & RobotModel::get_urdf_model() const {
  std::call_once(urdf_model_flag_, [&]{
    urdf_model_ = std::make_unique<urdf::Model>();
    urdf_model_->initString(get_robot_description());
  });

  assert(urdf_model_);
  return *urdf_model_;
}

const JointMapBuilder & RobotModel::get_joint_map_builder() const {
  std::call_once(joint_map_builder_flag_, [&]{
    joint_map_builder_ = std::make_unique<JointMapBuilder>();

    joint_map_builder_->with_urdf(get_urdf_model());
    joint_map_builder_->with_transmissions(get_robot_description(), rclcpp::get_logger("robot_model"));
  });

  assert(joint_map_builder_);
  return *joint_map_builder_;
}

const AnalysisTree & RobotModel::get_analysis_tree() const {
  std::call_once(analysis_tree_flag_, [&] {
    analysis_tree_ = std::make_unique<AnalysisTree>(get_urdf_model());
  });

  assert(analysis_tree_);
  return *analysis_tree_;
}

} // namespace arm_kinematics