/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE:     nova_twistmapper
AUTHOR:      Bailey Chessum
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "nova_twistmapper/nova_twistmapper.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <memory>
#include <string>
#include <type_traits>
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

namespace {

std::string format_collider_name(
  const std::size_t collider_index,
  const std::vector<std::string> & parent_link_names)
{
  if (collider_index >= parent_link_names.size() || parent_link_names[collider_index].empty()) {
    return "#" + std::to_string(collider_index);
  }

  return parent_link_names[collider_index] + "[" + std::to_string(collider_index) + "]";
}

std::string format_collision_pairs(
  const std::vector<std::pair<size_t, size_t>> & colliding_pairs,
  const std::vector<std::string> & parent_link_names)
{
  std::string formatted;
  for (std::size_t i = 0; i < colliding_pairs.size(); ++i) {
    if (!formatted.empty()) {
      formatted += ", ";
    }

    const auto & [a, b] = colliding_pairs[i];
    formatted += format_collider_name(a, parent_link_names);
    formatted += " <-> ";
    formatted += format_collider_name(b, parent_link_names);
  }

  return formatted;
}

}  // namespace

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

NovaTwistmapper::TwistmapperMode NovaTwistmapper::twistmapper_mode() const noexcept
{
  return params_.use_position_control ? TwistmapperMode::Position : TwistmapperMode::Velocity;
}

const char * NovaTwistmapper::joint_command_type() const noexcept
{
  return twistmapper_mode() == TwistmapperMode::Position ?
    hardware_interface::HW_IF_POSITION :
    hardware_interface::HW_IF_VELOCITY;
}

void NovaTwistmapper::log_self_intersection_pairs(
  const char * message_prefix,
  const std::vector<std::pair<size_t, size_t>> & colliding_pairs)
{
  const auto logger = get_node()->get_logger();

  if (colliding_pairs.empty()) {
    RCLCPP_ERROR_THROTTLE(
      logger,
      *get_node()->get_clock(),
      200,
      "Collision plugin invariant violation: %s was rejected for self-intersection without collision pair details.",
      message_prefix);
    return;
  }

  const auto formatted_pairs = format_collision_pairs(
    colliding_pairs,
    kinematics_->collision_manager.parent_link_names());

  RCLCPP_WARN_THROTTLE(
    logger,
    *get_node()->get_clock(),
    200,
    "%s self intersects. Colliding pairs: %s",
    message_prefix,
    formatted_pairs.c_str());
}

InterfaceConfiguration NovaTwistmapper::command_interface_configuration() const
{
  return {
    interface_configuration_type::INDIVIDUAL,
    arm_kinematics::ros2_control::command_interface_names(
      params_.joint_names,
      joint_command_type(),
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
  assert(joint_values.size() == registered_joint_handles_.size());
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

NovaTwistmapper::ResolvedTwist NovaTwistmapper::resolve_base_twist(
  const std::vector<double> & seed_state,
  const rclcpp::Time & time)
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
    return {TwistResolutionStatus::NoMessage, arm_kinematics::Twistd::Zero()};
  }

  const rclcpp::Time command_time{twist_stamped->header.stamp, time.get_clock_type()};
  const double command_age = (time - command_time).seconds();
  if (!std::isfinite(command_age) || command_age < 0.0 || command_age > params_.cmd_timeout) {
    RCLCPP_WARN_THROTTLE(
      logger,
      *get_node()->get_clock(),
      2000,
      "Latest TwistStamped command is stale or invalid: age %.6f seconds, timeout %.6f seconds.",
      command_age,
      params_.cmd_timeout);
    return {TwistResolutionStatus::StaleMessage, arm_kinematics::Twistd::Zero()};
  }

  arm_kinematics::ForwardKinematicsPlugin::Tree::SharedPtr twist_frame_tree;
  active_twist_frame_tree_.get(twist_frame_tree);
  if (!twist_frame_tree) {
    RCLCPP_ERROR(logger, "Twist frame tree was not configured.");
    return {TwistResolutionStatus::InvalidFrame, arm_kinematics::Twistd::Zero()};
  }

  twist_frame_tree->position_fk(seed_state, fk_pose_buffer_);
  const Eigen::Isometry3d & twist_frame = fk_pose_buffer_.front();

  if (params_.publish_debug_frames) {
    publish_to_tf2(
      time,
      twist_frame,
      debug_twist_frame_child_frame_id_);
  }

  arm_kinematics::Twistd local_twist;
  Eigen::fromMsg(twist_stamped->twist, local_twist);

  arm_kinematics::Twistd base_twist = local_twist;
  base_twist.linear() = twist_frame.linear() * local_twist.linear();
  base_twist.angular() = twist_frame.linear() * local_twist.angular();

  return {TwistResolutionStatus::Valid, base_twist};
}

Eigen::Isometry3d NovaTwistmapper::integrate_target_pose(
  const arm_kinematics::Twistd & base_twist,
  const rclcpp::Duration & period,
  const Eigen::Isometry3d & current_target_pose) const
{
  return arm_kinematics::apply_twist(base_twist, period.seconds(), current_target_pose);
}

bool NovaTwistmapper::write_commands(const std::vector<double> & commands)
{
  const auto logger = get_node()->get_logger();
  if (commands.size() != registered_joint_handles_.size()) {
    RCLCPP_ERROR(
      logger,
      "Refusing to write %zu commands for %zu registered joints.",
      commands.size(),
      registered_joint_handles_.size());
    return false;
  }

  for (std::size_t i = 0; i < commands.size(); ++i) {
    if (!registered_joint_handles_[i].command.get().set_value(commands[i])) {
      RCLCPP_ERROR(
        logger,
        "Failed to write %s command for joint '%s'.",
        joint_command_type(),
        registered_joint_handles_[i].name.c_str());
      return false;
    }
  }

  return true;
}

bool NovaTwistmapper::write_zero_velocity_commands()
{
  std::vector<double> zero_commands(registered_joint_handles_.size(), 0.0);
  return write_commands(zero_commands);
}

controller_interface::return_type NovaTwistmapper::update_position_mode(
  PositionRuntime & runtime,
  const rclcpp::Time & time,
  const rclcpp::Duration & period)
{
  const auto logger = get_node()->get_logger();

  const auto resolved_twist = resolve_base_twist(current_joint_state_values_, time);
  if (resolved_twist.status != TwistResolutionStatus::Valid) {
    publish_to_tf2(time, runtime.target_pose);
    return controller_interface::return_type::OK;
  }

  const auto candidate_pose = integrate_target_pose(
    resolved_twist.base_twist,
    period,
    runtime.target_pose);

  auto ik_result = kinematics_->ik->get_position_ik(
    candidate_pose,
    current_joint_state_values_,
    runtime.solution_positions);
  if (!ik_result) {
    RCLCPP_WARN_THROTTLE(
      logger,
      *get_node()->get_clock(),
      200,
      "Failed to find solution to inverse kinematics: %s",
      ik_result.error().format().c_str());
    publish_to_tf2(time, runtime.target_pose);
    return controller_interface::return_type::OK;
  }

  if (runtime.solution_positions.size() != current_joint_state_values_.size()) {
    RCLCPP_ERROR(
      logger,
      "IK plugin returned %zu joints, but twistmapper expects %zu.",
      runtime.solution_positions.size(),
      current_joint_state_values_.size());
    publish_to_tf2(time, runtime.target_pose);
    return controller_interface::return_type::OK;
  }

  if (!vector_is_finite(runtime.solution_positions)) {
    RCLCPP_ERROR(logger, "IK plugin returned NaN or Inf joint values.");
    publish_to_tf2(time, runtime.target_pose);
    return controller_interface::return_type::OK;
  }

  const auto collision_result = arm_kinematics::check_path_collision(
    kinematics_->collision_manager,
    current_joint_state_values_,
    runtime.solution_positions,
    params_.self_intersection_max_step_size,
    joint_values_scratch_,
    colliding_pairs_scratch_);
  if (!collision_result) {
    RCLCPP_ERROR(
      logger,
      "Failed to check path collision: %s",
      collision_result.error().format().c_str());
    publish_to_tf2(time, runtime.target_pose);
    return controller_interface::return_type::OK;
  }
  if (*collision_result) {
    log_self_intersection_pairs("Inverse Kinematics solution", colliding_pairs_scratch_);
    publish_to_tf2(time, runtime.target_pose);
    return controller_interface::return_type::OK;
  }

  if (!write_commands(runtime.solution_positions)) {
    return controller_interface::return_type::ERROR;
  }

  runtime.target_pose = candidate_pose;
  publish_to_tf2(time, runtime.target_pose);
  return controller_interface::return_type::OK;
}

controller_interface::return_type NovaTwistmapper::update_velocity_mode(
  VelocityRuntime & runtime,
  const rclcpp::Time & time,
  const rclcpp::Duration & period)
{
  const auto logger = get_node()->get_logger();
  kinematics_->ee_tree->position_fk(current_joint_state_values_, fk_pose_buffer_);
  const Eigen::Isometry3d current_ee_pose = fk_pose_buffer_.front();
  runtime.current_ee_pose = current_ee_pose;

  const double dt = period.seconds();

  if (!std::isfinite(dt) || dt <= 0.0 || dt > params_.max_velocity_control_period) {
    RCLCPP_ERROR_THROTTLE(
      logger,
      *get_node()->get_clock(),
      2000,
      "Velocity mode requires a finite positive period no larger than %.6f seconds, got %.9f.",
      params_.max_velocity_control_period,
      dt);

    if (!write_zero_velocity_commands())
      return controller_interface::return_type::ERROR;

    publish_to_tf2(time, runtime.current_ee_pose);
    return controller_interface::return_type::OK;
  }

  const auto resolved_twist = resolve_base_twist(current_joint_state_values_, time);
  if (resolved_twist.status != TwistResolutionStatus::Valid) {
    if (!write_zero_velocity_commands()) {
      return controller_interface::return_type::ERROR;
    }
    publish_to_tf2(time, runtime.current_ee_pose);
    return controller_interface::return_type::OK;
  }

  auto ik_result = kinematics_->ik->get_velocity_ik(
    resolved_twist.base_twist,
    current_ee_pose,
    current_joint_state_values_,
    runtime.solution_velocities,
    params_.use_control_period_for_velocity_ik ? dt : params_.velocity_ik_time_step);

  if (!ik_result) {
    RCLCPP_WARN_THROTTLE(
      logger,
      *get_node()->get_clock(),
      200,
      "Failed to find solution to inverse velocity kinematics: %s",
      ik_result.error().format().c_str());
    if (!write_zero_velocity_commands()) {
      return controller_interface::return_type::ERROR;
    }
    publish_to_tf2(time, runtime.current_ee_pose);
    return controller_interface::return_type::OK;
  }

  if (runtime.solution_velocities.size() != current_joint_state_values_.size()) {
    RCLCPP_ERROR(
      logger,
      "Velocity IK plugin returned %zu joints, but twistmapper expects %zu.",
      runtime.solution_velocities.size(),
      current_joint_state_values_.size());
    if (!write_zero_velocity_commands()) {
      return controller_interface::return_type::ERROR;
    }
    publish_to_tf2(time, runtime.current_ee_pose);
    return controller_interface::return_type::OK;
  }

  if (!vector_is_finite(runtime.solution_velocities)) {
    RCLCPP_ERROR(logger, "Velocity IK plugin returned NaN or Inf joint values.");
    if (!write_zero_velocity_commands()) {
      return controller_interface::return_type::ERROR;
    }
    publish_to_tf2(time, runtime.current_ee_pose);
    return controller_interface::return_type::OK;
  }

  for (std::size_t i = 0; i < predicted_joint_positions_.size(); ++i) {
    predicted_joint_positions_[i] =
      current_joint_state_values_[i] + runtime.solution_velocities[i] * dt;
  }

  if (!vector_is_finite(predicted_joint_positions_)) {
    RCLCPP_ERROR(logger, "Predicted next joint positions contain NaN or Inf.");
    if (!write_zero_velocity_commands()) {
      return controller_interface::return_type::ERROR;
    }
    publish_to_tf2(time, runtime.current_ee_pose);
    return controller_interface::return_type::OK;
  }

  const auto collision_result = arm_kinematics::check_path_collision(
    kinematics_->collision_manager,
    current_joint_state_values_,
    predicted_joint_positions_,
    params_.self_intersection_max_step_size,
    joint_values_scratch_,
    colliding_pairs_scratch_);
  if (!collision_result) {
    RCLCPP_ERROR(
      logger,
      "Failed to check path collision: %s",
      collision_result.error().format().c_str());
    if (!write_zero_velocity_commands()) {
      return controller_interface::return_type::ERROR;
    }
    publish_to_tf2(time, runtime.current_ee_pose);
    return controller_interface::return_type::OK;
  }
  if (*collision_result) {
    log_self_intersection_pairs("Velocity IK solution", colliding_pairs_scratch_);
    if (!write_zero_velocity_commands()) {
      return controller_interface::return_type::ERROR;
    }
    publish_to_tf2(time, runtime.current_ee_pose);
    return controller_interface::return_type::OK;
  }

  if (!write_commands(runtime.solution_velocities)) {
    return controller_interface::return_type::ERROR;
  }

  publish_to_tf2(time, runtime.current_ee_pose);
  return controller_interface::return_type::OK;
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
  if (!mode_runtime_) {
    RCLCPP_ERROR(logger, "Twistmapper update() called before runtime mode was configured.");
    return controller_interface::return_type::ERROR;
  }

  read_state_pos_values(current_joint_state_values_);

  if (auto * position_runtime = std::get_if<PositionRuntime>(&*mode_runtime_)) {
    return update_position_mode(*position_runtime, time, period);
  }

  return update_velocity_mode(std::get<VelocityRuntime>(*mode_runtime_), time, period);
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
  if (!std::isfinite(params_.self_intersection_max_step_size) ||
    params_.self_intersection_max_step_size <= 0.0)
  {
    RCLCPP_ERROR(logger, "self_intersection_max_step_size must be finite and > 0.");
    return controller_interface::CallbackReturn::ERROR;
  }
  if (!std::isfinite(params_.cmd_timeout) || params_.cmd_timeout <= 0.0) {
    RCLCPP_ERROR(logger, "cmd_timeout must be finite and > 0.");
    return controller_interface::CallbackReturn::ERROR;
  }
  if (!std::isfinite(params_.velocity_ik_time_step) || params_.velocity_ik_time_step <= 0.0) {
    RCLCPP_ERROR(logger, "velocity_ik_time_step must be finite and > 0.");
    return controller_interface::CallbackReturn::ERROR;
  }
  if (!std::isfinite(params_.max_velocity_control_period) ||
    params_.max_velocity_control_period <= 0.0)
  {
    RCLCPP_ERROR(logger, "max_velocity_control_period must be finite and > 0.");
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
  debug_twist_frame_child_frame_id_ = std::string(get_node()->get_name()) + "_twist_frame";

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
  const auto collider_count = kinematics_->collision_manager.parent_link_names().size();
  colliding_pairs_scratch_.reserve(collider_count > 1 ? collider_count * (collider_count - 1) / 2 : 0);

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
  joint_values_scratch_.assign(params_.joint_names.size(), 0.0);
  predicted_joint_positions_.assign(params_.joint_names.size(), 0.0);

  if (twistmapper_mode() == TwistmapperMode::Position) {
    PositionRuntime runtime;
    runtime.solution_positions.assign(params_.joint_names.size(), 0.0);
    mode_runtime_ = std::move(runtime);
  }
  else {
    VelocityRuntime runtime;
    runtime.solution_velocities.assign(params_.joint_names.size(), 0.0);
    mode_runtime_ = std::move(runtime);
  }
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
          received_twist_stamped_ptr_.set(nullptr);
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
    joint_command_type(),
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
  if (!mode_runtime_) {
    RCLCPP_ERROR(logger, "Runtime mode was not configured before activation.");
    return controller_interface::CallbackReturn::ERROR;
  }

  is_halted = false;
  subscriber_is_active_ = true;

  read_state_pos_values(current_joint_state_values_);
  kinematics_->ee_tree->position_fk(current_joint_state_values_, fk_pose_buffer_);
  std::visit(
    [this](auto & runtime) {
      if constexpr (std::is_same_v<std::decay_t<decltype(runtime)>, PositionRuntime>) {
        runtime.target_pose = fk_pose_buffer_.front();
      } else {
        runtime.current_ee_pose = fk_pose_buffer_.front();
      }
    },
    *mode_runtime_);

  const std::string frame_id = last_frame_id_.empty() ? params_.fallback_frame_id : last_frame_id_;
  auto twist_tree_result = make_single_frame_tree(frame_id);
  if (!twist_tree_result) {
    RCLCPP_ERROR(
      logger,
      "Failed to rebuild FK tree for active twist frame '%s': %s",
      frame_id.c_str(),
      twist_tree_result.error().c_str());
    return controller_interface::CallbackReturn::ERROR;
  }
  active_twist_frame_tree_.set(std::move(*twist_tree_result));
  last_frame_id_ = frame_id;

  if (twistmapper_mode() == TwistmapperMode::Position) {
    for (auto & joint : registered_joint_handles_) {
      if (!joint.command.get().set_value(joint.state_pos.get().get_value())) {
        return controller_interface::CallbackReturn::ERROR;
      }
    }
  } else {
    if (!write_zero_velocity_commands()) {
      return controller_interface::CallbackReturn::ERROR;
    }
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
  debug_twist_frame_child_frame_id_.clear();

  active_twist_frame_tree_.set(nullptr);
  received_twist_stamped_ptr_.set(nullptr);
  kinematics_.reset();

  twistmapper_pose_tf_broadcaster_.reset();
  twist_stamped_sub_.reset();
  subscriber_is_active_ = false;
  registered_joint_handles_.clear();
  current_joint_state_values_.clear();
  joint_values_scratch_.clear();
  predicted_joint_positions_.clear();
  colliding_pairs_scratch_.clear();
  mode_runtime_.reset();
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

  if (registered_joint_handles_.empty()) {
    return;
  }

  if (twistmapper_mode() == TwistmapperMode::Position) {
    for (auto & joint : registered_joint_handles_) {
      (void)joint.command.get().set_value(joint.state_pos.get().get_value());
    }
    return;
  }

  (void)write_zero_velocity_commands();
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
