/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE:     nova_twistmapper
AUTHOR:      Bailey Chessum
EDITED BY:   Codex
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "nova_twistmapper/nova_twistmapper.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "class_loader/register_macro.hpp"
#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "lifecycle_msgs/msg/state.hpp"
#include "rclcpp/logging.hpp"
#include "tf2_eigen/tf2_eigen.hpp"
#include "tf2_geometry_msgs/tf2_geometry_msgs/tf2_geometry_msgs.hpp"

#include "arm_kinematics/collision/collision_utilities.hpp"
#include "arm_kinematics/forward/frame_definitions.hpp"
#include "arm_kinematics/joint_map/state_interface_definition.hpp"
#include "arm_kinematics/ros2_control/interface_names.hpp"
#include "arm_kinematics/ros2_control/interface_refs.hpp"
#include "arm_kinematics/utilities/utilities.hpp"

using std::placeholders::_1;

namespace nova_twistmapper
{
using controller_interface::interface_configuration_type;
using controller_interface::InterfaceConfiguration;
using lifecycle_msgs::msg::State;

NovaTwistmapper::NovaTwistmapper()
: controller_interface::ControllerInterface()
{
}

controller_interface::CallbackReturn NovaTwistmapper::on_init()
{
  try {
    param_listener_ = std::make_shared<ParamListener>(get_node());
    params_ = param_listener_->get_params();
  }
  catch (const std::exception & e) {
    fprintf(stderr, "Exception thrown during init stage with message: %s\n", e.what());
    return controller_interface::CallbackReturn::ERROR;
  }

  return controller_interface::CallbackReturn::SUCCESS;
}

InterfaceConfiguration NovaTwistmapper::command_interface_configuration() const
{
  return {
    interface_configuration_type::INDIVIDUAL,
    arm_kinematics::ros2_control::command_interface_names(
      params_.joint_names,
      hardware_interface::HW_IF_POSITION,
      params_.chained_controller_name),
  };
}

InterfaceConfiguration NovaTwistmapper::state_interface_configuration() const
{
  return {
    interface_configuration_type::INDIVIDUAL,
    arm_kinematics::ros2_control::state_interface_names(
      params_.joint_names,
      {hardware_interface::HW_IF_POSITION}),
  };
}

void NovaTwistmapper::read_state_pos_values(std::vector<double> & joint_values) const
{
  joint_values.resize(registered_joint_handles_.size());
  for (std::size_t i = 0; i < registered_joint_handles_.size(); ++i) {
    joint_values[i] = registered_joint_handles_[i].state_pos.get().get_value();
  }
}

tl::expected<arm_kinematics::ForwardKinematicsPlugin::Tree::SharedPtr, std::string>
NovaTwistmapper::make_single_frame_tree(const std::string & frame_name) const
{
  if (!kinematics_ || !kinematics_->fk) {
    return tl::unexpected("Cannot build FK tree before the FK plugin has been created.");
  }

  std::vector<arm_kinematics::NamedStateInterfaceDefinition> named_inputs;
  named_inputs.reserve(params_.joint_names.size());
  for (const auto & joint_name : params_.joint_names) {
    named_inputs.emplace_back(joint_name, arm_kinematics::InterfaceId::Position());
  }

  auto tree_result = kinematics_->fk->make_tree(
    arm_kinematics::span<const arm_kinematics::NamedStateInterfaceDefinition>(
      named_inputs.data(), named_inputs.size()),
    params_.base_link_name,
    arm_kinematics::FrameDefinitions{frame_name});
  if (!tree_result) {
    return tl::unexpected(tree_result.error().format());
  }

  return arm_kinematics::ForwardKinematicsPlugin::Tree::SharedPtr(
    std::move(tree_result.value().tree));
}

Eigen::Isometry3d NovaTwistmapper::integrate_twist(
  const std::vector<double> & seed_state,
  const rclcpp::Duration & period,
  const Eigen::Isometry3d & current_target_pose)
{
  const auto logger = get_node()->get_logger();

  std::shared_ptr<geometry_msgs::msg::TwistStamped> twist_stamped;
  received_twist_stamped_ptr_.get(twist_stamped);

  if (!twist_stamped) {
    RCLCPP_WARN_THROTTLE(
      logger,
      *get_node()->get_clock(),
      2000,
      "Haven't yet received a TwistStamped message to use for the twistmapper.");
    return current_target_pose;
  }

  arm_kinematics::ForwardKinematicsPlugin::Tree::SharedPtr twist_frame_tree;
  active_twist_frame_tree_.get(twist_frame_tree);
  if (!twist_frame_tree) {
    RCLCPP_ERROR(logger, "Twist frame tree was not configured.");
    return current_target_pose;
  }

  twist_frame_tree->position_fk(seed_state, fk_pose_buffer_);
  const Eigen::Isometry3d & twist_frame = fk_pose_buffer_.front();

  if (params_.publish_debug_frames) {
    publish_to_tf2(
      get_node()->get_clock()->now(),
      twist_frame,
      std::string(get_node()->get_name()) + "_twist_frame");
  }

  arm_kinematics::Twistd local_twist;
  Eigen::fromMsg(twist_stamped->twist, local_twist);

  arm_kinematics::Twistd base_twist = local_twist;
  base_twist.block<3, 1>(0, 0) = twist_frame.linear() * local_twist.block<3, 1>(0, 0);
  base_twist.block<3, 1>(3, 0) = twist_frame.linear() * local_twist.block<3, 1>(3, 0);

  return arm_kinematics::apply_twist(base_twist, period.seconds(), current_target_pose);
}

controller_interface::return_type NovaTwistmapper::update(
  const rclcpp::Time & time,
  const rclcpp::Duration & period)
{
  const auto logger = get_node()->get_logger();
  if (get_lifecycle_state().id() == State::PRIMARY_STATE_INACTIVE) {
    if (!is_halted) {
      halt();
      is_halted = true;
    }
    return controller_interface::return_type::OK;
  }

  if (!kinematics_ || !kinematics_->ik) {
    RCLCPP_ERROR(logger, "Twistmapper update() called before kinematics were configured.");
    return controller_interface::return_type::ERROR;
  }

  read_state_pos_values(current_joint_state_values_);

  const auto candidate_pose = integrate_twist(current_joint_state_values_, period, twistmapper_pose_);

  auto ik_result = kinematics_->ik->get_position_ik(
    candidate_pose,
    current_joint_state_values_,
    ik_solution_);
  if (!ik_result) {
    RCLCPP_WARN_THROTTLE(
      logger,
      *get_node()->get_clock(),
      200,
      "Failed to find solution to inverse kinematics: %s",
      ik_result.error().format().c_str());
    publish_to_tf2(time, twistmapper_pose_);
    return controller_interface::return_type::OK;
  }

  if (ik_solution_.size() != current_joint_state_values_.size()) {
    RCLCPP_ERROR(
      logger,
      "IK plugin returned %zu joints, but twistmapper expects %zu.",
      ik_solution_.size(),
      current_joint_state_values_.size());
    publish_to_tf2(time, twistmapper_pose_);
    return controller_interface::return_type::OK;
  }

  if (!vector_is_finite(ik_solution_)) {
    RCLCPP_ERROR(logger, "IK plugin returned NaN or Inf joint values.");
    publish_to_tf2(time, twistmapper_pose_);
    return controller_interface::return_type::OK;
  }

  if (arm_kinematics::check_path_collision(
        kinematics_->collision_manager,
        current_joint_state_values_,
        ik_solution_,
        params_.self_intersection_max_step_size))
  {
    RCLCPP_WARN_THROTTLE(
      logger,
      *get_node()->get_clock(),
      200,
      "Inverse Kinematics solution self intersects.");
    publish_to_tf2(time, twistmapper_pose_);
    return controller_interface::return_type::OK;
  }

  for (std::size_t i = 0; i < ik_solution_.size(); ++i) {
    (void)registered_joint_handles_[i].command.get().set_value(ik_solution_[i]);
  }

  twistmapper_pose_ = candidate_pose;
  publish_to_tf2(time, twistmapper_pose_);

  return controller_interface::return_type::OK;
}

controller_interface::CallbackReturn NovaTwistmapper::on_configure(const rclcpp_lifecycle::State &)
{
  const auto logger = get_node()->get_logger();
  RCLCPP_INFO(logger, "On configure");

  if (param_listener_->is_old(params_)) {
    params_ = param_listener_->get_params();
  }

  if (params_.joint_names.empty()) {
    RCLCPP_ERROR(logger, "Joint names parameter is empty.");
    return controller_interface::CallbackReturn::ERROR;
  }
  if (params_.base_link_name.empty()) {
    RCLCPP_ERROR(logger, "base_link_name parameter is empty.");
    return controller_interface::CallbackReturn::ERROR;
  }
  if (params_.ee_link_name.empty()) {
    RCLCPP_ERROR(logger, "ee_link_name parameter is empty.");
    return controller_interface::CallbackReturn::ERROR;
  }
  if (params_.fallback_frame_id.empty()) {
    RCLCPP_ERROR(logger, "fallback_frame_id parameter is empty.");
    return controller_interface::CallbackReturn::ERROR;
  }
  if (params_.self_intersection_max_step_size <= 0.0) {
    RCLCPP_ERROR(logger, "self_intersection_max_step_size must be > 0.");
    return controller_interface::CallbackReturn::ERROR;
  }

  if (!reset()) {
    return controller_interface::CallbackReturn::ERROR;
  }

  const std::string robot_description = get_robot_description();
  if (robot_description.empty()) {
    RCLCPP_ERROR(logger, "robot_description is empty.");
    return controller_interface::CallbackReturn::ERROR;
  }

  twistmapper_pose_tf_broadcaster_ =
    std::make_shared<tf2_ros::TransformBroadcaster>(tf2_ros::TransformBroadcaster(*get_node()));

  kinematics_.emplace();
  kinematics_->loader = arm_kinematics::PluginLoader{*get_node(), robot_description};

  kinematics_->fk = kinematics_->loader.make_fk();
  if (!kinematics_->fk) {
    RCLCPP_ERROR(logger, "Failed to create FK plugin.");
    return controller_interface::CallbackReturn::ERROR;
  }

  kinematics_->ik = kinematics_->loader.make_ik();
  if (!kinematics_->ik) {
    RCLCPP_ERROR(logger, "Failed to create IK plugin.");
    return controller_interface::CallbackReturn::ERROR;
  }

  auto collision_config = arm_kinematics::read_collision_config(
    get_node()->get_node_parameters_interface());
  auto collision_result = arm_kinematics::make_collision_manager(
    kinematics_->loader,
    kinematics_->fk,
    params_.joint_names,
    collision_config);
  if (!collision_result) {
    RCLCPP_ERROR(logger, "Failed to build collision manager: %s", collision_result.error().format().c_str());
    return controller_interface::CallbackReturn::ERROR;
  }
  kinematics_->collision_manager = std::move(*collision_result);

  auto ee_tree_result = make_single_frame_tree(params_.ee_link_name);
  if (!ee_tree_result) {
    RCLCPP_ERROR(
      logger,
      "Failed to build FK tree for ee_link_name '%s': %s",
      params_.ee_link_name.c_str(),
      ee_tree_result.error().c_str());
    return controller_interface::CallbackReturn::ERROR;
  }
  kinematics_->ee_tree = std::move(*ee_tree_result);

  auto twist_tree_result = make_single_frame_tree(params_.fallback_frame_id);
  if (!twist_tree_result) {
    RCLCPP_ERROR(
      logger,
      "Failed to build FK tree for fallback_frame_id '%s': %s",
      params_.fallback_frame_id.c_str(),
      twist_tree_result.error().c_str());
    return controller_interface::CallbackReturn::ERROR;
  }
  active_twist_frame_tree_.set(std::move(*twist_tree_result));
  last_frame_id_ = params_.fallback_frame_id;

  current_joint_state_values_.assign(params_.joint_names.size(), 0.0);
  ik_solution_.assign(params_.joint_names.size(), 0.0);
  received_twist_stamped_ptr_.set(nullptr);

  twist_stamped_sub_ = get_node()->create_subscription<geometry_msgs::msg::TwistStamped>(
    params_.twist_stamped_topic,
    rclcpp::SystemDefaultsQoS(),
    [this](std::shared_ptr<geometry_msgs::msg::TwistStamped> msg) -> void
    {
      const auto logger = get_node()->get_logger();
      RCLCPP_INFO_ONCE(logger, "Twist message received.");

      if (!subscriber_is_active_) {
        RCLCPP_WARN_ONCE(logger, "Can't accept new commands. subscriber is inactive");
        return;
      }

      if ((msg->header.stamp.sec == 0) && (msg->header.stamp.nanosec == 0)) {
        RCLCPP_WARN_ONCE(
          logger,
          "Received message with zero timestamp, setting it to current time.");
        msg->header.stamp = get_node()->get_clock()->now();
      }

      const std::string new_frame_id =
        msg->header.frame_id.empty() ? params_.fallback_frame_id : msg->header.frame_id;

      if (new_frame_id != last_frame_id_) {
        auto tree_result = make_single_frame_tree(new_frame_id);
        if (!tree_result) {
          RCLCPP_WARN_THROTTLE(
            logger,
            *get_node()->get_clock(),
            2000,
            "Rejecting TwistStamped for unsupported frame_id '%s': %s",
            new_frame_id.c_str(),
            tree_result.error().c_str());
          return;
        }

        active_twist_frame_tree_.set(std::move(*tree_result));
        last_frame_id_ = new_frame_id;
      }

      if (msg->header.frame_id.empty()) {
        msg->header.frame_id = new_frame_id;
      }
      received_twist_stamped_ptr_.set(std::move(msg));
    });
  subscriber_is_active_ = true;

  previous_update_timestamp_ = get_node()->get_clock()->now();
  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn NovaTwistmapper::configure_joints()
{
  const auto logger = get_node()->get_logger();
  RCLCPP_INFO(logger, "Configure joints");

  if (params_.joint_names.empty()) {
    RCLCPP_ERROR(logger, "No joint names specified");
    return controller_interface::CallbackReturn::ERROR;
  }

  if (!registered_joint_handles_.empty()) {
    RCLCPP_ERROR(logger, "registered_joint_handles_ was not empty. Clearing stale handles.");
    registered_joint_handles_.clear();
  }

  std::vector<arm_kinematics::NamedStateInterfaceDefinition> state_defs;
  state_defs.reserve(params_.joint_names.size());
  for (const auto & joint_name : params_.joint_names) {
    state_defs.emplace_back(joint_name, arm_kinematics::InterfaceId::Position());
  }

  auto state_refs_result = arm_kinematics::ros2_control::find_state_interface_refs(
    state_interfaces_,
    arm_kinematics::span<const arm_kinematics::NamedStateInterfaceDefinition>(
      state_defs.data(), state_defs.size()));
  if (!state_refs_result) {
    state_refs_result.error().log(logger);
    return controller_interface::CallbackReturn::ERROR;
  }

  const auto command_names = arm_kinematics::ros2_control::command_interface_names(
    params_.joint_names,
    hardware_interface::HW_IF_POSITION,
    params_.chained_controller_name);
  auto command_refs_result = arm_kinematics::ros2_control::find_command_interface_refs(
    command_interfaces_,
    arm_kinematics::span<const std::string>(command_names.data(), command_names.size()));
  if (!command_refs_result) {
    command_refs_result.error().log(logger);
    return controller_interface::CallbackReturn::ERROR;
  }

  registered_joint_handles_.reserve(params_.joint_names.size());
  for (std::size_t i = 0; i < params_.joint_names.size(); ++i) {
    registered_joint_handles_.push_back(JointHandle{
      params_.joint_names[i],
      std::cref(state_refs_result.value()[i].get()),
      std::ref(command_refs_result.value()[i].get()),
    });
  }

  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn NovaTwistmapper::on_activate(const rclcpp_lifecycle::State &)
{
  const auto logger = get_node()->get_logger();
  RCLCPP_INFO(logger, "On activate");

  const auto joints_result = configure_joints();
  if (joints_result == controller_interface::CallbackReturn::ERROR) {
    RCLCPP_ERROR(logger, "Some joint interfaces are non existent");
    return controller_interface::CallbackReturn::ERROR;
  }

  if (!kinematics_ || !kinematics_->ee_tree) {
    RCLCPP_ERROR(logger, "Kinematics were not configured before activation.");
    return controller_interface::CallbackReturn::ERROR;
  }

  is_halted = false;
  subscriber_is_active_ = true;

  read_state_pos_values(current_joint_state_values_);
  kinematics_->ee_tree->position_fk(current_joint_state_values_, fk_pose_buffer_);
  twistmapper_pose_ = fk_pose_buffer_.front();

  for (auto & joint : registered_joint_handles_) {
    (void)joint.command.get().set_value(joint.state_pos.get().get_value());
  }

  RCLCPP_INFO(logger, "Initial twistmapper pose set from forward kinematics.");
  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn NovaTwistmapper::on_deactivate(const rclcpp_lifecycle::State &)
{
  RCLCPP_INFO(get_node()->get_logger(), "On deactivate");
  subscriber_is_active_ = false;
  if (!is_halted) {
    halt();
    is_halted = true;
  }

  registered_joint_handles_.clear();
  active_twist_frame_tree_.set(nullptr);

  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn NovaTwistmapper::on_cleanup(const rclcpp_lifecycle::State &)
{
  RCLCPP_INFO(get_node()->get_logger(), "On cleanup");

  if (!reset()) {
    return controller_interface::CallbackReturn::ERROR;
  }

  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn NovaTwistmapper::on_error(const rclcpp_lifecycle::State &)
{
  RCLCPP_INFO(get_node()->get_logger(), "On error");
  if (!reset()) {
    return controller_interface::CallbackReturn::ERROR;
  }
  return controller_interface::CallbackReturn::SUCCESS;
}

bool NovaTwistmapper::reset()
{
  RCLCPP_INFO(get_node()->get_logger(), "Resetting twistmapper state.");

  is_halted = false;
  last_frame_id_.clear();

  active_twist_frame_tree_.set(nullptr);
  received_twist_stamped_ptr_.set(nullptr);
  kinematics_.reset();

  twistmapper_pose_tf_broadcaster_.reset();
  twist_stamped_sub_.reset();
  subscriber_is_active_ = false;
  registered_joint_handles_.clear();
  current_joint_state_values_.clear();
  ik_solution_.clear();
  twistmapper_pose_ = Eigen::Isometry3d::Identity();
  fk_pose_buffer_.assign(1, Eigen::Isometry3d::Identity());

  return true;
}

controller_interface::CallbackReturn NovaTwistmapper::on_shutdown(const rclcpp_lifecycle::State &)
{
  return controller_interface::CallbackReturn::SUCCESS;
}

void NovaTwistmapper::halt()
{
  auto twist_stamped = std::make_shared<geometry_msgs::msg::TwistStamped>();
  twist_stamped->header.frame_id = params_.fallback_frame_id;
  twist_stamped->header.stamp = get_node()->get_clock()->now();
  received_twist_stamped_ptr_.set(std::move(twist_stamped));
}

void NovaTwistmapper::publish_to_tf2(
  const rclcpp::Time & time,
  const Eigen::Isometry3d & pose,
  const std::string & name)
{
  if (!twistmapper_pose_tf_broadcaster_) {
    return;
  }

  auto transform_stamped = tf2::eigenToTransform(pose);
  transform_stamped.header.stamp = time;
  transform_stamped.child_frame_id =
    name.empty() ? params_.kinematics_output_target_frame : name;
  transform_stamped.header.frame_id = params_.base_link_name;

  RCLCPP_INFO_ONCE(
    get_node()->get_logger(),
    "Broadcasting twistmapper pose as '%s', child of '%s'.",
    transform_stamped.child_frame_id.c_str(),
    transform_stamped.header.frame_id.c_str());

  twistmapper_pose_tf_broadcaster_->sendTransform(transform_stamped);
}

bool NovaTwistmapper::vector_is_finite(const std::vector<double> & values) const noexcept
{
  return std::all_of(
    values.begin(),
    values.end(),
    [](double value) { return std::isfinite(value); });
}

}  // namespace nova_twistmapper

CLASS_LOADER_REGISTER_CLASS(
  nova_twistmapper::NovaTwistmapper,
  controller_interface::ControllerInterface)
