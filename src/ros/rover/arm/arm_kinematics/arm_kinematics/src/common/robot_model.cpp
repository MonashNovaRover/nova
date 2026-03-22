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

const TransmissionAnalysis & RobotModel::get_default_transmission_analysis() const {
  std::call_once(default_transmission_analysis_flag_, [&]{
    default_transmission_analysis_ = std::make_unique<TransmissionAnalysis>();

    add_mimic_transmissions_to_analysis(*default_transmission_analysis_, get_urdf_model());
    add_ros2_control_transmissions_to_analysis(
      *default_transmission_analysis_,
      get_robot_description(),
      rclcpp::get_logger("robot_model"));
  });

  assert(default_transmission_analysis_);
  return *default_transmission_analysis_;
}

const AnalysisTree & RobotModel::get_analysis_tree() const {
  std::call_once(analysis_tree_flag_, [&] {
    analysis_tree_ = std::make_unique<AnalysisTree>(get_urdf_model());
  });

  assert(analysis_tree_);
  return *analysis_tree_;
}

} // namespace arm_kinematics
