/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE:     nova_path_planner
AUTHOR:      Bailey Chessum
EDITED BY:   Codex
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "nova_path_planner/nova_path_planner.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "lifecycle_msgs/msg/state.hpp"
#include "pluginlib/class_list_macros.hpp"
#include "rclcpp/logging.hpp"
#include "tf2_eigen/tf2_eigen.hpp"
#include "tf2_geometry_msgs/tf2_geometry_msgs.hpp"

#include "arm_kinematics/collision/collision_utilities.hpp"
#include "arm_kinematics/forward/frame_definitions.hpp"
#include "arm_kinematics/joint_map/state_interface_definition.hpp"
#include "arm_kinematics/ros2_control/interface_names.hpp"
#include "arm_kinematics/ros2_control/interface_refs.hpp"

using std::placeholders::_1;
using std::placeholders::_2;

namespace nova_path_planner
{
using controller_interface::interface_configuration_type;
using controller_interface::InterfaceConfiguration;
using lifecycle_msgs::msg::State;

NovaPathPlanner::NovaPathPlanner()
: controller_interface::ControllerInterface()
{
}

controller_interface::CallbackReturn NovaPathPlanner::on_init()
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

InterfaceConfiguration NovaPathPlanner::command_interface_configuration() const
{
  return {
    interface_configuration_type::INDIVIDUAL,
    arm_kinematics::ros2_control::command_interface_names(
      params_.joint_names,
      hardware_interface::HW_IF_POSITION,
      params_.chained_controller_name),
  };
}

InterfaceConfiguration NovaPathPlanner::state_interface_configuration() const
{
  return {
    interface_configuration_type::INDIVIDUAL,
    arm_kinematics::ros2_control::state_interface_names(
      params_.joint_names,
      {hardware_interface::HW_IF_POSITION}),
  };
}

controller_interface::return_type NovaPathPlanner::update(
  const rclcpp::Time &,
  const rclcpp::Duration &)
{
  const auto logger = get_node()->get_logger();
  if (get_lifecycle_state().id() == State::PRIMARY_STATE_INACTIVE) {
    if (!is_halted) {
      halt();
      is_halted = true;
    }
    return controller_interface::return_type::OK;
  }

  if (clear_path_requested_.exchange(false, std::memory_order_acq_rel)) {
    pending_path_ptr_.set(nullptr);
    active_path_.reset();
    active_path_index_ = 0;
    remaining_path_points_.store(0, std::memory_order_release);
    is_path_being_executed_.store(false, std::memory_order_release);
    return controller_interface::return_type::OK;
  }

  if (!active_path_) {
    std::shared_ptr<const PlannedPath> pending_path;
    pending_path_ptr_.get(pending_path);
    if (pending_path) {
      active_path_ = std::move(pending_path);
      active_path_index_ = 0;
      pending_path_ptr_.set(nullptr);
      remaining_path_points_.store(active_path_->points.size(), std::memory_order_release);
    }
  }

  if (!active_path_ || active_path_->points.empty()) {
    RCLCPP_INFO_THROTTLE(logger, *get_node()->get_clock(), 2000, "Executor has no points to send.");
    return controller_interface::return_type::OK;
  }

  if (active_path_index_ >= active_path_->points.size()) {
    active_path_.reset();
    active_path_index_ = 0;
    remaining_path_points_.store(0, std::memory_order_release);
    is_path_being_executed_.store(false, std::memory_order_release);
    return controller_interface::return_type::OK;
  }

  const auto remaining_before_pop = active_path_->points.size() - active_path_index_;
  const auto & command = active_path_->points[active_path_index_];
  ++active_path_index_;

  RCLCPP_INFO_THROTTLE(
    logger,
    *get_node()->get_clock(),
    10,
    "Sending path point to the command interfaces. (%zu points remaining before consume)",
    remaining_before_pop);

  for (std::size_t i = 0; i < command.size(); ++i) {
    (void)registered_joint_handles_[i].command.get().set_value(command[i]);
  }

  const auto remaining_after_pop = active_path_->points.size() - active_path_index_;
  remaining_path_points_.store(remaining_after_pop, std::memory_order_release);
  if (remaining_after_pop == 0) {
    active_path_.reset();
    active_path_index_ = 0;
    is_path_being_executed_.store(false, std::memory_order_release);
  }

  return controller_interface::return_type::OK;
}

controller_interface::CallbackReturn NovaPathPlanner::on_configure(const rclcpp_lifecycle::State &)
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
  if (params_.kinematics_output_target_frame.empty()) {
    RCLCPP_ERROR(logger, "kinematics_output_target_frame parameter is empty.");
    return controller_interface::CallbackReturn::ERROR;
  }
  if (params_.action_name.empty()) {
    RCLCPP_ERROR(logger, "action_name parameter is empty.");
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

  target_pose_tf_broadcaster_ =
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
    RCLCPP_ERROR(
      logger,
      "Failed to build collision manager: %s",
      collision_result.error().format().c_str());
    return controller_interface::CallbackReturn::ERROR;
  }
  kinematics_->collision_manager = std::move(*collision_result);

  std::vector<arm_kinematics::NamedStateInterfaceDefinition> named_inputs;
  named_inputs.reserve(params_.joint_names.size());
  for (const auto & joint_name : params_.joint_names) {
    named_inputs.emplace_back(joint_name, arm_kinematics::InterfaceId::Position());
  }

  auto ee_tree_result = kinematics_->fk->make_tree(
    arm_kinematics::span<const arm_kinematics::NamedStateInterfaceDefinition>(
      named_inputs.data(), named_inputs.size()),
    params_.base_link_name,
    arm_kinematics::FrameDefinitions{params_.ee_link_name});
  if (!ee_tree_result) {
    RCLCPP_ERROR(
      logger,
      "Failed to build FK tree for ee_link_name '%s': %s",
      params_.ee_link_name.c_str(),
      ee_tree_result.error().format().c_str());
    return controller_interface::CallbackReturn::ERROR;
  }
  kinematics_->ee_tree = arm_kinematics::ForwardKinematicsPlugin::Tree::SharedPtr(
    std::move(ee_tree_result->tree));

  current_joint_state_values_.assign(params_.joint_names.size(), 0.0);
  ik_solution_.assign(params_.joint_names.size(), 0.0);
  pending_path_ptr_.set(nullptr);
  active_path_.reset();
  active_path_index_ = 0;
  clear_path_requested_.store(false, std::memory_order_release);
  remaining_path_points_.store(0, std::memory_order_release);
  has_target_pose_ = false;

  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn NovaPathPlanner::configure_joints()
{
  const auto logger = get_node()->get_logger();
  RCLCPP_INFO(logger, "Configure joints");

  if (params_.joint_names.empty()) {
    RCLCPP_ERROR(logger, "No joint names specified.");
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

controller_interface::CallbackReturn NovaPathPlanner::on_activate(const rclcpp_lifecycle::State &)
{
  const auto logger = get_node()->get_logger();
  RCLCPP_INFO(logger, "On activate");

  const auto joints_result = configure_joints();
  if (joints_result == controller_interface::CallbackReturn::ERROR) {
    RCLCPP_ERROR(logger, "Some joint interfaces are non existent.");
    return controller_interface::CallbackReturn::ERROR;
  }

  if (!kinematics_ || !kinematics_->ee_tree || !kinematics_->ik) {
    RCLCPP_ERROR(logger, "Kinematics were not configured before activation.");
    return controller_interface::CallbackReturn::ERROR;
  }

  is_halted = false;

  read_state_pos_values(current_joint_state_values_);
  kinematics_->ee_tree->position_fk(current_joint_state_values_, fk_pose_buffer_);

  for (auto & joint : registered_joint_handles_) {
    (void)joint.command.get().set_value(joint.state_pos.get().get_value());
  }

  action_server_ = rclcpp_action::create_server<ArmPlanPath>(
    get_node(),
    params_.action_name,
    std::bind(&NovaPathPlanner::handle_action_goal, this, _1, _2),
    std::bind(&NovaPathPlanner::handle_action_cancelled, this, _1),
    std::bind(&NovaPathPlanner::handle_action_accepted, this, _1));

  if (has_target_pose_) {
    publish_target_pose_to_tf2(get_node()->get_clock()->now(), target_pose_);
  }

  RCLCPP_INFO(logger, "Path planner action server created.");
  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn NovaPathPlanner::on_deactivate(const rclcpp_lifecycle::State &)
{
  RCLCPP_INFO(get_node()->get_logger(), "On deactivate");
  if (!is_halted) {
    halt();
    is_halted = true;
  }

  action_server_.reset();
  pending_path_ptr_.set(nullptr);
  active_path_.reset();
  active_path_index_ = 0;
  clear_path_requested_.store(false, std::memory_order_release);
  remaining_path_points_.store(0, std::memory_order_release);
  is_path_being_executed_.store(false, std::memory_order_release);
  registered_joint_handles_.clear();

  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn NovaPathPlanner::on_cleanup(const rclcpp_lifecycle::State &)
{
  RCLCPP_INFO(get_node()->get_logger(), "On cleanup");
  if (!reset()) {
    return controller_interface::CallbackReturn::ERROR;
  }
  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn NovaPathPlanner::on_error(const rclcpp_lifecycle::State &)
{
  RCLCPP_INFO(get_node()->get_logger(), "On error");
  if (!reset()) {
    return controller_interface::CallbackReturn::ERROR;
  }
  return controller_interface::CallbackReturn::SUCCESS;
}

bool NovaPathPlanner::reset()
{
  RCLCPP_INFO(get_node()->get_logger(), "Resetting path planner state.");

  is_halted = false;
  action_server_.reset();
  pending_path_ptr_.set(nullptr);
  active_path_.reset();
  active_path_index_ = 0;
  registered_joint_handles_.clear();
  current_joint_state_values_.clear();
  ik_solution_.clear();
  fk_pose_buffer_.assign(1, Eigen::Isometry3d::Identity());
  kinematics_.reset();
  clear_path_requested_.store(false, std::memory_order_release);
  remaining_path_points_.store(0, std::memory_order_release);
  is_path_being_executed_.store(false, std::memory_order_release);
  target_pose_tf_broadcaster_.reset();
  target_pose_ = Eigen::Isometry3d::Identity();
  has_target_pose_ = false;

  return true;
}

controller_interface::CallbackReturn NovaPathPlanner::on_shutdown(const rclcpp_lifecycle::State &)
{
  return controller_interface::CallbackReturn::SUCCESS;
}

void NovaPathPlanner::halt()
{
  clear_path_execution();
}

void NovaPathPlanner::read_state_pos_values(std::vector<double> & joint_values) const
{
  joint_values.resize(registered_joint_handles_.size());
  for (std::size_t i = 0; i < registered_joint_handles_.size(); ++i) {
    joint_values[i] = registered_joint_handles_[i].state_pos.get().get_value();
  }
}

std::vector<double> NovaPathPlanner::get_state_pos_values_non_rt() const
{
  std::vector<double> joint_values;
  joint_values.resize(registered_joint_handles_.size());
  for (std::size_t i = 0; i < registered_joint_handles_.size(); ++i) {
    joint_values[i] = registered_joint_handles_[i].state_pos.get().get_value();
  }
  return joint_values;
}

void NovaPathPlanner::publish_target_pose_to_tf2(
  const rclcpp::Time & time,
  const Eigen::Isometry3d & pose)
{
  if (!target_pose_tf_broadcaster_) {
    return;
  }

  auto transform_stamped = tf2::eigenToTransform(pose);
  transform_stamped.header.stamp = time;
  transform_stamped.child_frame_id = params_.kinematics_output_target_frame;
  transform_stamped.header.frame_id = params_.base_link_name;

  RCLCPP_INFO_ONCE(
    get_node()->get_logger(),
    "Broadcasting path planner target pose as '%s', child of '%s'.",
    transform_stamped.child_frame_id.c_str(),
    transform_stamped.header.frame_id.c_str());

  target_pose_tf_broadcaster_->sendTransform(transform_stamped);
}

rclcpp_action::GoalResponse NovaPathPlanner::handle_action_goal(
  const rclcpp_action::GoalUUID & uuid,
  std::shared_ptr<const ArmPlanPath::Goal>)
{
  const auto logger = get_node()->get_logger();
  (void)uuid;

  bool expected = false;
  if (!is_path_being_executed_.compare_exchange_strong(expected, true)) {
    RCLCPP_ERROR(logger, "Goal rejected because a path is already being executed.");
    return rclcpp_action::GoalResponse::REJECT;
  }

  RCLCPP_INFO(logger, "Received and accepted goal request.");
  return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
}

void NovaPathPlanner::handle_action_accepted(const std::shared_ptr<GoalHandleArmPlanPath> & goal_handle)
{
  RCLCPP_INFO(get_node()->get_logger(), "Spinning up new thread to handle the action.");
  std::thread{std::bind(&NovaPathPlanner::execute_action, this, _1), goal_handle}.detach();
}

rclcpp_action::CancelResponse NovaPathPlanner::handle_action_cancelled(
  const std::shared_ptr<GoalHandleArmPlanPath> & goal_handle)
{
  RCLCPP_INFO(get_node()->get_logger(), "Received request to cancel goal.");
  (void)goal_handle;
  return rclcpp_action::CancelResponse::ACCEPT;
}

void NovaPathPlanner::execute_action(std::shared_ptr<GoalHandleArmPlanPath> goal_handle)
{
  const auto logger = get_node()->get_logger();
  const auto goal = goal_handle->get_goal();
  auto result = std::make_shared<ArmPlanPath::Result>();
  auto feedback = std::make_shared<ArmPlanPath::Feedback>();

  std::vector<double> last_joint_pose = get_state_pos_values_non_rt();

  Eigen::Isometry3d start = Eigen::Isometry3d::Identity();
  if (!try_get_pose_from_forward_kinematics(last_joint_pose, start)) {
    clear_path_execution();
    result->success = false;
    goal_handle->succeed(result);
    return;
  }

  if (!(goal->speed > 0.0)) {
    RCLCPP_ERROR(logger, "Goal rejected internally because speed must be > 0.");
    clear_path_execution();
    result->success = false;
    goal_handle->succeed(result);
    return;
  }

  Eigen::Isometry3d end = Eigen::Isometry3d::Identity();
  Eigen::fromMsg(goal->pose, end);

  feedback->traversing_path = false;
  feedback->path_generation_progress = 0.0;
  goal_handle->publish_feedback(feedback);

  if (!generate_path(start, end, last_joint_pose, goal->speed)) {
    clear_path_execution();
    result->success = false;
    goal_handle->succeed(result);
    return;
  }

  feedback->traversing_path = true;
  feedback->path_generation_progress = 1.0;
  goal_handle->publish_feedback(feedback);

  while (is_path_being_executed_.load(std::memory_order_acquire)) {
    if (goal_handle->is_canceling()) {
      clear_path_execution();
      goal_handle->canceled(result);
      RCLCPP_INFO(logger, "Goal canceled.");
      return;
    }

    if (!rclcpp::ok()) {
      clear_path_execution();
      return;
    }

    std::this_thread::yield();
  }

  clear_path_execution();
  result->success = true;
  goal_handle->succeed(result);
  RCLCPP_INFO(logger, "Goal succeeded.");
}

bool NovaPathPlanner::try_get_pose_from_forward_kinematics(
  const std::vector<double> & joint_positions,
  Eigen::Isometry3d & result)
{
  if (!kinematics_ || !kinematics_->ee_tree) {
    RCLCPP_ERROR(get_node()->get_logger(), "Forward kinematics tree was not configured.");
    return false;
  }
  if (!vector_is_finite(joint_positions)) {
    RCLCPP_ERROR(get_node()->get_logger(), "Cannot run FK on NaN or Inf joint values.");
    return false;
  }

  kinematics_->ee_tree->position_fk(joint_positions, fk_pose_buffer_);
  if (fk_pose_buffer_.empty()) {
    RCLCPP_ERROR(get_node()->get_logger(), "FK tree returned no poses.");
    return false;
  }

  result = fk_pose_buffer_.front();
  if (!result.matrix().allFinite()) {
    RCLCPP_ERROR(get_node()->get_logger(), "FK returned a pose containing NaN or Inf.");
    return false;
  }

  return true;
}

bool NovaPathPlanner::generate_path(
  const Eigen::Isometry3d & start,
  const Eigen::Isometry3d & end,
  std::vector<double> & last_pushed_pose,
  const double speed)
{
  const auto logger = get_node()->get_logger();
  if (!kinematics_ || !kinematics_->ik) {
    RCLCPP_ERROR(logger, "Cannot generate a path before kinematics are configured.");
    return false;
  }
  if (!(speed > 0.0)) {
    RCLCPP_ERROR(logger, "generate_path() requires speed > 0.");
    return false;
  }

  const Eigen::Isometry3d & handle0 = start;
  const Eigen::Isometry3d handle1 = end;

  const double distance = (end.translation() - start.translation()).norm();
  const double execution_time = distance / speed;
  const double frequency = static_cast<double>(get_update_rate());
  const int pose_count = std::max(2, static_cast<int>(std::floor(execution_time * frequency)) + 1);
  const int pose_count_minus_one = pose_count - 1;

  auto path = std::make_shared<PlannedPath>();
  path->points.reserve(static_cast<std::size_t>(pose_count));

  RCLCPP_INFO(
    logger,
    "Generating %d path points for distance %.6fm and execution time %.6fs.",
    pose_count,
    distance,
    execution_time);

  for (int i = 0; i < pose_count; ++i) {
    const double ik_t = static_cast<double>(i) / static_cast<double>(pose_count_minus_one);
    const Eigen::Isometry3d pose = lerp3(start, handle0, handle1, end, ik_t);

    auto ik_result = kinematics_->ik->get_position_ik(pose, last_pushed_pose, ik_solution_);
    if (!ik_result) {
      RCLCPP_ERROR(
        logger,
        "Failed to find IK solution at t=%f: %s",
        ik_t,
        ik_result.error().format().c_str());
      return false;
    }

    if (ik_solution_.size() != last_pushed_pose.size()) {
      RCLCPP_ERROR(
        logger,
        "IK plugin returned %zu joints, but path planner expects %zu.",
        ik_solution_.size(),
        last_pushed_pose.size());
      return false;
    }

    if (!vector_is_finite(ik_solution_)) {
      RCLCPP_ERROR(logger, "IK plugin returned NaN or Inf joint values.");
      return false;
    }

    const auto collision_result = arm_kinematics::check_path_collision(
      kinematics_->collision_manager,
      last_pushed_pose,
      ik_solution_,
      params_.self_intersection_max_step_size);
    if (!collision_result) {
      RCLCPP_ERROR(
        logger,
        "Failed to check path collision at t=%f: %s",
        ik_t,
        collision_result.error().format().c_str());
      return false;
    }
    if (*collision_result) {
      RCLCPP_ERROR(logger, "IK segment self-intersects at t=%f.", ik_t);
      return false;
    }

    last_pushed_pose = ik_solution_;
    path->points.push_back(ik_solution_);
    std::this_thread::yield();
  }

  target_pose_ = end;
  has_target_pose_ = true;
  publish_target_pose_to_tf2(get_node()->get_clock()->now(), target_pose_);
  remaining_path_points_.store(path->points.size(), std::memory_order_release);
  pending_path_ptr_.set(std::move(path));
  RCLCPP_INFO(logger, "Finished generating the path.");
  return true;
}

void NovaPathPlanner::clear_path_execution()
{
  pending_path_ptr_.set(nullptr);
  clear_path_requested_.store(true, std::memory_order_release);
  if (remaining_path_points_.load(std::memory_order_acquire) == 0) {
    clear_path_requested_.store(false, std::memory_order_release);
    is_path_being_executed_.store(false, std::memory_order_release);
  }
}

bool NovaPathPlanner::vector_is_finite(const std::vector<double> & values) const noexcept
{
  return std::all_of(values.begin(), values.end(), [](const double value) {
    return std::isfinite(value);
  });
}

inline Eigen::Vector3d NovaPathPlanner::lerp(const Vector3d & a, const Vector3d & b, const double & t)
{
  return (1.0 - t) * a + t * b;
}

inline Eigen::Vector3d NovaPathPlanner::lerp2(
  const Vector3d & a,
  const Vector3d & b,
  const Vector3d & c,
  const double & t)
{
  return lerp(lerp(a, b, t), lerp(b, c, t), t);
}

inline Eigen::Vector3d NovaPathPlanner::lerp3(
  const Vector3d & a,
  const Vector3d & b,
  const Vector3d & c,
  const Vector3d & d,
  const double & t)
{
  return lerp(lerp2(a, b, c, t), lerp2(b, c, d, t), t);
}

inline Eigen::Quaterniond NovaPathPlanner::slerp(
  const Eigen::Quaterniond & a,
  const Eigen::Quaterniond & b,
  const double & t)
{
  return a.slerp(t, b);
}

inline Eigen::Quaterniond NovaPathPlanner::slerp2(
  const Eigen::Quaterniond & a,
  const Eigen::Quaterniond & b,
  const Eigen::Quaterniond & c,
  const double & t)
{
  return slerp(slerp(a, b, t), slerp(b, c, t), t);
}

inline Eigen::Quaterniond NovaPathPlanner::slerp3(
  const Eigen::Quaterniond & a,
  const Eigen::Quaterniond & b,
  const Eigen::Quaterniond & c,
  const Eigen::Quaterniond & d,
  const double & t)
{
  return slerp(slerp2(a, b, c, t), slerp2(b, c, d, t), t);
}

inline Eigen::Isometry3d NovaPathPlanner::lerp3(
  Eigen::Isometry3d a,
  Eigen::Isometry3d b,
  Eigen::Isometry3d c,
  Eigen::Isometry3d d,
  const double t)
{
  Eigen::Isometry3d transform = Eigen::Isometry3d::Identity();

  transform.translation() = lerp3(
    a.translation(),
    b.translation(),
    c.translation(),
    d.translation(),
    t);

  transform.linear() = slerp3(
    Eigen::Quaterniond(a.linear()),
    Eigen::Quaterniond(b.linear()),
    Eigen::Quaterniond(c.linear()),
    Eigen::Quaterniond(d.linear()),
    t).toRotationMatrix();

  return transform;
}

}  // namespace nova_path_planner

PLUGINLIB_EXPORT_CLASS(
  nova_path_planner::NovaPathPlanner,
  controller_interface::ControllerInterface)
