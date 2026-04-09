//
// Created by Bailey Chessum on 15/12/2025.
//

#include "arm_kinematics/common/robot_model.hpp"

#include <urdf/model.h>
#include <rclcpp/logging.hpp>

#include "arm_kinematics/forward/utilities/analysis_tree.hpp"
#include "arm_kinematics/joint_map/ros2_control_transmission_plugin_loader.hpp"
#include "arm_kinematics/joint_map/transmission_analysis.hpp"
#include "arm_kinematics/joint_map/transmission_analysis_import.hpp"

namespace arm_kinematics {

RobotModel::RobotModel(std::string robot_description)
: robot_description_(std::move(robot_description))
{
}

// Out-of-line: required so the LazyOnce<T>-managed forward-declared types only need to be
// complete here (where the includes above bring in their full definitions), not in every
// translation unit that includes robot_model.hpp.
RobotModel::~RobotModel() = default;

const std::string & RobotModel::get_robot_description() const {
  return robot_description_;
}

const urdf::Model & RobotModel::get_urdf_model() const {
  return urdf_model_.get([this]() {
    auto model = std::make_unique<urdf::Model>();
    model->initString(get_robot_description());
    return model;
  });
}

const TransmissionAnalysis & RobotModel::get_default_transmission_analysis() const {
  return default_transmission_analysis_.get([this]() {
    auto analysis = std::make_unique<TransmissionAnalysis>();
    add_mimic_transmissions_to_analysis(*analysis, get_urdf_model());
    (void)add_ros2_control_transmissions_to_analysis(
      *analysis,
      get_robot_description(),
      get_ros2_control_transmission_plugin_loader(),
      rclcpp::get_logger("robot_model"));
    return analysis;
  });
}

std::shared_ptr<const Ros2ControlTransmissionPluginLoader> RobotModel::get_ros2_control_transmission_plugin_loader() const
{
  std::call_once(ros2_control_transmission_plugin_loader_flag_, [&] {
    ros2_control_transmission_plugin_loader_ = std::make_shared<Ros2ControlTransmissionPluginLoader>();
  });

  assert(ros2_control_transmission_plugin_loader_);
  return ros2_control_transmission_plugin_loader_;
}

const AnalysisTree & RobotModel::get_analysis_tree() const {
  return analysis_tree_.get([this]() {
    return std::make_unique<AnalysisTree>(get_urdf_model());
  });
}

} // namespace arm_kinematics
