/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	  nova_path_planner
AUTHOR:     Bailey Chessum
EDITED BY:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "nova_path_planner/nova_path_planner.hpp"
#include "lifecycle_msgs/msg/state.hpp"
#include "rclcpp/logging.hpp"
#include "tf2_geometry_msgs/tf2_geometry_msgs/tf2_geometry_msgs.hpp"
#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "tf2_eigen/tf2_eigen.hpp"
#include <urdf_parser/urdf_parser.h>
#include <srdfdom/model.h>
#include <pluginlib/class_loader.hpp>
#include <moveit/kinematics_base/kinematics_base.h>
#include <moveit/utils/moveit_error_code.h>


namespace
{
  constexpr auto DEFAULT_INPUT_TOPIC_END_EFFECTOR_TWIST = "/arm_ik_twist_stamped"; // TODO: changeme
} // namespace

using std::placeholders::_1;
using std::placeholders::_2;

namespace nova_path_planner
{
  using namespace std::chrono_literals;
  using controller_interface::interface_configuration_type;
  using controller_interface::InterfaceConfiguration;
  using lifecycle_msgs::msg::State;


  NovaPathPlanner::NovaPathPlanner() : controller_interface::ControllerInterface() {}

  controller_interface::CallbackReturn NovaPathPlanner::on_init()
  {
    try
    {
      // Create the parameter listener and get the parameters
      param_listener_ = std::make_shared<ParamListener>(get_node());
      params_ = param_listener_->get_params();

      kinematics_solver_loader_ = std::make_unique<pluginlib::ClassLoader<kinematics::KinematicsBase>>(
        "moveit_core", "kinematics::KinematicsBase");
//      planner_loader_ = std::make_unique<pluginlib::ClassLoader<planning_interface::PlannerManager>>(
//        "moveit_core", "planning_interface::PlannerManager");
    }
    catch (const std::exception &e)
    {
      fprintf(stderr, "Exception thrown during init stage with message: %s \n", e.what());
      return controller_interface::CallbackReturn::ERROR;
    }

    return controller_interface::CallbackReturn::SUCCESS;
  }

  InterfaceConfiguration NovaPathPlanner::command_interface_configuration() const
  {
    std::vector<std::string> conf_names;

    for (const auto &joint : params_.joint_names)
    {
      conf_names.push_back(joint_to_command_interface_name(joint));
    }

    return {interface_configuration_type::INDIVIDUAL, conf_names};
  }

  InterfaceConfiguration NovaPathPlanner::state_interface_configuration() const
  {
    std::vector<std::string> conf_names;

    for (const auto &joint_name: params_.joint_names) {
      conf_names.push_back(joint_name + "/" + hardware_interface::HW_IF_POSITION);
    }

    return {interface_configuration_type::INDIVIDUAL, conf_names};
  }

  void NovaPathPlanner::update_path_planner_pose(const rclcpp::Time &time, const rclcpp::Duration &period) {
    std::shared_ptr<geometry_msgs::msg::TwistStamped> twist_stamped;

    if (twist_stamped == nullptr) {
      RCLCPP_WARN(get_node()->get_logger(), "Haven't yet received a TwistStamped message to use for the path_planner.");
      return;
    }

    const auto& twist = twist_stamped->twist;

    tf2::Vector3 tf2_twist_angular;
    tf2::fromMsg(twist.angular, tf2_twist_angular);

    // Create rotation matrix from twist.angular as XYZ euler
    auto rotation_matrix = tf2::Matrix3x3();

    const auto linear = tf2::Vector3(
      twist.linear.x * period.seconds(),
      twist.linear.y * period.seconds(),
      twist.linear.z * period.seconds());

    // TODO: Test this actually works as expected.
    // TODO: Add a tf buffer and make use of the header.frame_id from the twist_stamped to allow IK to be done relative to anything. Then Teleop would just need to use the end effector frame by default
    // TODO: If using tf like this, also have the path_planner broadcast the target frame, so it can reference itself.
    path_planner_pose_.setOrigin(linear + path_planner_pose_.getOrigin());
    path_planner_pose_.setBasis(rotation_matrix);
  }

  controller_interface::return_type NovaPathPlanner::update(const rclcpp::Time &time, const rclcpp::Duration &period)
  {
    auto logger = get_node()->get_logger();
    if (get_lifecycle_state().id() == State::PRIMARY_STATE_INACTIVE)
    {
      if (!is_halted)
      {
        halt();
        is_halted = true;
      }

      return controller_interface::return_type::OK;
    }

    auto old_pose = path_planner_pose_;

    // Get a new pose
    update_path_planner_pose(time, period);
    publish_to_tf2(time);

    // Do IK to find the joint values for that pose
    std::vector<double> joint_state_values = get_state_pos_values();
    std::vector<double> solution;
    geometry_msgs::msg::Pose pose = tf2::toMsg(tf2::transformToEigen(tf2::toMsg(path_planner_pose_)));
    moveit_msgs::msg::MoveItErrorCodes error_codes;

    //auto result = kinematics_solver_->getPositionIK(pose, joint_state_values, solution, error_codes);
    auto result = kinematics_solver_->searchPositionIK(pose, joint_state_values, params_.kinematics_solver_timeout, solution, error_codes);
    if (!result) {
      path_planner_pose_ = old_pose;
      RCLCPP_WARN_THROTTLE(logger, *get_node()->get_clock(), 500,
                           "Failed to find solution to inverse kinematics: error code %d (\"%s\"). %s", error_codes.val,
                           moveit::core::errorCodeToString(error_codes).c_str(), error_codes.message.c_str());

      return controller_interface::return_type::OK;
    }

    if (check_path_for_self_intersection(joint_state_values, solution)) {
      path_planner_pose_ = old_pose;
      RCLCPP_WARN_THROTTLE(logger, *get_node()->get_clock(), 500, "Inverse Kinematics solution self intersects!");
      return controller_interface::return_type::OK;
    }

    // Apply solution to command interfaces
    for (size_t i = 0; i < solution.size(); i++) {
      registered_joint_handles_[i].command.get().set_value(solution[i]);
    }

    return controller_interface::return_type::OK;
  }

  controller_interface::CallbackReturn NovaPathPlanner::on_configure(const rclcpp_lifecycle::State&)
  {
    auto logger = get_node()->get_logger();
    RCLCPP_INFO(logger, "On configure");

    // update parameters if they have changed
    if (param_listener_->is_old(params_))
    {
      params_ = param_listener_->get_params();
    }

    // TODO: maybe limit position so arm doesn't collide?
    if (!reset())
    {
      return controller_interface::CallbackReturn::ERROR;
    }

    path_planner_pose_tf_broadcaster_ = std::make_shared<tf2_ros::TransformBroadcaster>(tf2_ros::TransformBroadcaster(*get_node()));
    // tf_buffer_ = std::make_unique<tf2_ros::Buffer>(get_node()->get_clock());
    // tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

    subscriber_is_active_ = true;

    RCLCPP_INFO(logger, "Created twist stamped subscription");

    // Parse URDF
    std::string urdf_str = params_.robot_description;

    if (urdf_str.empty()) {
      RCLCPP_DEBUG(get_node()->get_logger(), "No URDF was provided in robot_description. Loading from "
                                            "get_robot_description() instead.");
      urdf_str = get_robot_description();
    }
    else {
      RCLCPP_DEBUG(logger, "Found URDF string from robot_description parameter.");
    }

    urdf_model_ = urdf::parseURDF(urdf_str);
    if (!urdf_model_) {
      RCLCPP_ERROR(get_node()->get_logger(), "Failed to parse the given robot_description URDF string \"%s\"",
                   urdf_str.c_str());
      return CallbackReturn::FAILURE;
    }

    // Create an SRDF with a joint group for params_.joint_names
    joint_group_name_ = params_.kinematics_solver_group_name;
    if (joint_group_name_.empty()) {
      joint_group_name_ = std::basic_string(get_node()->get_name()) + "_joints";
      RCLCPP_DEBUG(get_node()->get_logger(), "No kinematics_solver_group_name was specified. Using \"%s\".",
                  joint_group_name_.c_str());
    }
    srdf_model_ = std::make_shared<srdf::Model>();
    auto srdf_string = params_.robot_description_semantic;
    if (srdf_string.empty()) {
      RCLCPP_DEBUG(get_node()->get_logger(), "No robot_description_semantic SRDF was specified. Making one up.");
      srdf_string = construct_srdf_fallback_string(urdf_model_, joint_group_name_);
    }
    srdf_model_->initString(*urdf_model_, srdf_string);

    // Create the robot model
    robot_model_ = std::make_shared<moveit::core::RobotModel>(urdf_model_, srdf_model_);

    // Create the planning scene
    planning_scene_ = std::make_shared<planning_scene::PlanningScene>(robot_model_);
    generate_allowed_collision_matrix();

    // Create an extra non-lifecycle node to allow us to initialize the kinematics solver
    kinematics_compat_node_ = create_compat_node_from_lifecycle(get_node());

    // Load kinematics
    try
    {
      RCLCPP_INFO(logger, "Attempting to find plugin for kinematics as one of the following plugins:");
      auto plugins = kinematics_solver_loader_->getDeclaredClasses();
      for (const auto& plugin : plugins) {
        RCLCPP_INFO(logger, "  - %s", plugin.c_str());
      }

      kinematics_solver_ = kinematics_solver_loader_->createSharedInstance(params_.kinematics_solver);
    }
    catch (const pluginlib::PluginlibException& ex)
    {
      RCLCPP_ERROR(logger, "Failed to load IK solver plugin \'%s\': %s", params_.kinematics_solver.c_str(), ex.what());
      return controller_interface::CallbackReturn::ERROR;
    }
    RCLCPP_INFO(logger, "Loaded kinematics plugin \'%s\'", params_.kinematics_solver.c_str());

    // Instantiate kinematics using the robot model
    const std::basic_string<char> base_frame = params_.kinematics_base_frame;
    const std::vector<std::basic_string<char>> tip_frames {
      params_.kinematics_endeffector_frame
    };
    // We currently don't use this. Reasonable values are in [0.01, 0.1] rads, and KDL uses 0.1 rads by default.
    double search_discretization = params_.kinematics_solver_search_discretization;

    kinematics_solver_->initialize(kinematics_compat_node_, *robot_model_.get(), joint_group_name_, base_frame, tip_frames,
                                   search_discretization);

    // Load planner
    /*
    try
    {
      RCLCPP_INFO(logger, "Attempting to find plugin for path planning as one of the following plugins:");
      auto plugins = planner_loader_->getDeclaredClasses();
      for (const auto& plugin : plugins) {
        RCLCPP_INFO(logger, "  - %s", plugin.c_str());
      }

      planner_ = planner_loader_->createSharedInstance(params_.planner);
    }
    catch (const pluginlib::PluginlibException& ex)
    {
      RCLCPP_ERROR(logger, "Failed to load IK solver plugin \'%s\': %s", params_.planner.c_str(), ex.what());
      return controller_interface::CallbackReturn::ERROR;
    }
    RCLCPP_INFO(logger, "Loaded planner plugin \'%s\'", params_.kinematics_solver.c_str());

    kinematics_compat_node_->declare_parameter

    planner_->initialize(robot_model_, kinematics_compat_node_, "");
     */

    previous_update_timestamp_ = get_node()->get_clock()->now();
    return controller_interface::CallbackReturn::SUCCESS;
  }

  std::string NovaPathPlanner::construct_srdf_fallback_string(const urdf::ModelInterfaceSharedPtr &urdf_model,
                                                              std::string joint_group_name) {
    std::ostringstream srdf_stream;
    srdf_stream << "<robot name=\"" << urdf_model->getName() << "\">\n";
    srdf_stream << "  <group name=\"" << joint_group_name << "\">\n";
    for (const auto& joint : params_.joint_names)
      srdf_stream << "    <joint name=\"" << joint << "\"/>\n";
    srdf_stream << "    <chain base_link=\"" << params_.kinematics_base_frame << "\" tip_link=\"" << params_.kinematics_endeffector_frame << "\" />\n";
    srdf_stream << "  </group>\n";
    srdf_stream << "</robot>\n";
    auto srdf_string = srdf_stream.str();
    return srdf_string;
  }

  controller_interface::CallbackReturn NovaPathPlanner::on_activate(const rclcpp_lifecycle::State&)
  {
    auto logger = get_node()->get_logger();
    RCLCPP_INFO(logger, "On activate");

    // Set up joint state interfaces
    const auto joints_result = configure_joints();
    if (joints_result == controller_interface::CallbackReturn::ERROR)
    {
      RCLCPP_ERROR(logger, "Some joint interfaces are non existent");
      return controller_interface::CallbackReturn::ERROR;
    }

    // Validate the joint group
    auto& joint_group_names = robot_model_->getJointModelGroup(joint_group_name_)->getActiveJointModelNames();
    for (auto& handle : registered_joint_handles_) {
      if (std::find(joint_group_names.begin(), joint_group_names.end(), handle.name) == joint_group_names.end()) {
        RCLCPP_ERROR(logger, "SRDF joint group \"%s\" doesn't contain the joint \"%s\".",
                     joint_group_name_.c_str(), handle.name.c_str());

        return CallbackReturn::FAILURE;
      }
    }

    // Reorder the joint handles to match the order of the joint group (FK/IK won't work without this)
    std::unordered_map<std::string, size_t> joint_name_to_index;
    for (size_t i = 0; i < joint_group_names.size(); ++i) {
      joint_name_to_index[joint_group_names[i]] = i;
    }
    std::sort(registered_joint_handles_.begin(), registered_joint_handles_.end(), [&](const JointHandle& a, const JointHandle& b) {
      return joint_name_to_index[a.name] < joint_name_to_index[b.name];
    });

    RCLCPP_INFO(logger, "New joint handle order:");
    for (auto& handle : registered_joint_handles_) {
      RCLCPP_INFO(logger, "  - %s", handle.name.c_str());
    }

    RCLCPP_INFO(logger, "SRDF Joint group joints:");
    for (auto& joint : joint_group_names) {
      RCLCPP_INFO(logger, "  - %s", joint.c_str());
    }

    is_halted = false;
    subscriber_is_active_ = true;

    if (!robot_model_ || !robot_model_->getRootJoint()) {
      RCLCPP_ERROR(get_node()->get_logger(), "robot_model_ is uninitialized or invalid!");
      return CallbackReturn::ERROR;
    }

    if (!kinematics_solver_) {
      RCLCPP_ERROR(get_node()->get_logger(), "kinematics_solver_ is uninitialized or invalid!");
      return CallbackReturn::ERROR;
    }

    /*
    if (!planner_) {
      RCLCPP_ERROR(get_node()->get_logger(), "planner_ is uninitialized or invalid!");
      return CallbackReturn::ERROR;
    }
    */

    auto joint_values = get_state_pos_values();
    std::vector<geometry_msgs::msg::Pose> poses;

    auto result = kinematics_solver_->getPositionFK({params_.kinematics_endeffector_frame}, joint_values, poses);

    if (!result) {
      RCLCPP_ERROR(get_node()->get_logger(), "Failed to do forward kinematics to find the end effector's initial pose");
      return CallbackReturn::ERROR;
    }
    if (poses.empty()) {
      RCLCPP_ERROR(get_node()->get_logger(), "No poses returned from forward kinematics!");
      return CallbackReturn::ERROR;
    }

    // Store result to path_planner_pose_
    tf2::fromMsg(poses[0], path_planner_pose_);

    // Set path_planner_pose_rpy_ to match
    double roll, pitch, yaw;
    path_planner_pose_.getBasis().getRPY(roll, pitch, yaw);

    // Set initial command interface values from state interface
    for (auto& joint : registered_joint_handles_) {
      joint.command.get().set_value(joint.state_pos.get().get_value());
    }

    // Create the action server
    action_server_ = rclcpp_action::create_server<ArmPlanPath>(
      kinematics_compat_node_,
      params_.action_name,
      std::bind(&nova_path_planner::NovaPathPlanner::handle_action_goal, this, _1, _2),
      std::bind(&nova_path_planner::NovaPathPlanner::handle_action_cancelled, this, _1),
      std::bind(&nova_path_planner::NovaPathPlanner::handle_action_accepted, this, _1));

    RCLCPP_INFO(get_node()->get_logger(), "Initial path_planner pose set from forward kinematics.");
    return controller_interface::CallbackReturn::SUCCESS;
  }

  controller_interface::CallbackReturn NovaPathPlanner::on_deactivate(const rclcpp_lifecycle::State&)
  {
    RCLCPP_INFO(get_node()->get_logger(), "On Deactivate");
    subscriber_is_active_ = false;
    if (!is_halted)
    {
      halt();
      is_halted = true;
    }

    // Clean up command/state interfaces
    registered_joint_handles_.clear();

    return controller_interface::CallbackReturn::SUCCESS;
  }

  controller_interface::CallbackReturn NovaPathPlanner::on_cleanup(const rclcpp_lifecycle::State&)
  {
    RCLCPP_INFO(get_node()->get_logger(), "On Cleanup");

    if (!reset())
    {
      return controller_interface::CallbackReturn::ERROR;
    }

    return controller_interface::CallbackReturn::SUCCESS;
  }

  controller_interface::CallbackReturn NovaPathPlanner::on_error(const rclcpp_lifecycle::State&)
  {
    RCLCPP_INFO(get_node()->get_logger(), "On Error");
    if (!reset())
    {
      return controller_interface::CallbackReturn::ERROR;
    }
    return controller_interface::CallbackReturn::SUCCESS;
  }

  bool NovaPathPlanner::reset()
  {
    RCLCPP_INFO(get_node()->get_logger(), "TEMP: Resetting.");

    is_halted = false;

    // Reset pointers
    kinematics_solver_.reset();
    // planner_.reset();
    robot_model_.reset();
    urdf_model_.reset();
    srdf_model_.reset();
    kinematics_compat_node_.reset();

    path_planner_pose_tf_broadcaster_.reset();
    // tf_buffer_.reset();
    // tf_listener_.reset();

    // Reset subscriptions
    subscriber_is_active_ = false;

    // Reset action server
    // TODO: Clean up any running action server thread
    action_server_.reset();
    is_path_being_executed_ = false;

    return true;
  }

  controller_interface::CallbackReturn NovaPathPlanner::on_shutdown(const rclcpp_lifecycle::State&)
  {
    return controller_interface::CallbackReturn::SUCCESS;
  }

  void NovaPathPlanner::halt()
  {
    // Set path_planner velocities to 0
    std::shared_ptr<geometry_msgs::msg::TwistStamped> twist_stamped;

    twist_stamped->twist.linear.x = 0;
    twist_stamped->twist.linear.y = 0;
    twist_stamped->twist.linear.z = 0;

    twist_stamped->twist.angular.x = 0;
    twist_stamped->twist.angular.y = 0;
    twist_stamped->twist.angular.z = 0;
  }

  void NovaPathPlanner::publish_to_tf2(const rclcpp::Time &time) {
    // Publish path_planner pose to tf2
    geometry_msgs::msg::TransformStamped transform_stamped;
    transform_stamped.transform = toMsg(path_planner_pose_);
    transform_stamped.header.stamp = time;

    // TODO: Parameterize
    transform_stamped.child_frame_id = params_.kinematics_output_target_frame;
    transform_stamped.header.frame_id = params_.kinematics_base_frame;

    RCLCPP_INFO_ONCE(get_node()->get_logger(), "Broadcasting path_planner pose as '%s', child of '%s'.",
                     transform_stamped.child_frame_id.c_str(),
                     transform_stamped.header.frame_id.c_str());

    path_planner_pose_tf_broadcaster_->sendTransform(transform_stamped);
  }

  controller_interface::CallbackReturn NovaPathPlanner::configure_joints() {
    auto logger = get_node()->get_logger();
    RCLCPP_INFO(logger, "Configure joints");

    if (params_.joint_names.empty())
    {
      RCLCPP_ERROR(logger, "No joint names specified");
      return controller_interface::CallbackReturn::ERROR;
    }

    // register handles
    if (!registered_joint_handles_.empty()) {
      RCLCPP_ERROR(logger, "registered_joint_handles_ was not empty! Ensure this is propeprly cleaned up.");
      registered_joint_handles_.clear();
    }

    registered_joint_handles_.reserve(params_.joint_names.size());
    for (const auto &joint_name : params_.joint_names)
    {
      const auto state_interface_name = joint_name + "/" + hardware_interface::HW_IF_POSITION;

      const auto state_handle = std::find_if(
        state_interfaces_.begin(), state_interfaces_.end(),
        [&state_interface_name](const auto &interface)
        {
          return interface.get_name() == state_interface_name;
        });

      if (state_handle == state_interfaces_.end())
      {
        RCLCPP_ERROR(logger, "Unable to obtain joint state handle '%s' for %s", hardware_interface::HW_IF_POSITION, joint_name.c_str());

        RCLCPP_ERROR(logger, "state_interfaces_:");
        for (const auto& state_interface : state_interfaces_) {
          RCLCPP_ERROR(logger, "  > interface_name: %s", state_interface.get_interface_name().c_str());
          RCLCPP_ERROR(logger, "    prefix_name: %s", state_interface.get_prefix_name().c_str());
          RCLCPP_ERROR(logger, "    name: %s", state_interface.get_name().c_str());
        }
        return controller_interface::CallbackReturn::ERROR;
      }

      const auto command_interface_name = joint_to_command_interface_name(joint_name);

      const auto command_handle = std::find_if(
        command_interfaces_.begin(), command_interfaces_.end(),
        [&joint_name, &logger, &command_interface_name](const auto &interface)
        {
          return interface.get_name() == command_interface_name;
        });

      if (command_handle == command_interfaces_.end())
      {
        RCLCPP_ERROR(logger, "Unable to obtain joint command handle '%s' for %s", command_interface_name.c_str(), joint_name.c_str());

        RCLCPP_ERROR(logger, "command_interfaces_:");
        for (const auto& command_interface : command_interfaces_) {
          RCLCPP_ERROR(logger, "  > interface_name: %s", command_interface.get_interface_name().c_str());
          RCLCPP_ERROR(logger, "    prefix_name: %s", command_interface.get_prefix_name().c_str());
          RCLCPP_ERROR(logger, "    name: %s", command_interface.get_name().c_str());
        }
        return controller_interface::CallbackReturn::ERROR;
      }

      registered_joint_handles_.emplace_back(
        JointHandle{joint_name, std::ref(*state_handle), std::ref(*command_handle)});
    }

    return controller_interface::CallbackReturn::SUCCESS;
  }

  std::vector<double> NovaPathPlanner::get_state_pos_values() {
    std::vector<double> joint_values;

    joint_values.reserve(registered_joint_handles_.size());
    for (auto& joint_handle : registered_joint_handles_)
      joint_values.emplace_back(joint_handle.state_pos.get().get_value());
    return joint_values;
  }

  rclcpp::Node::SharedPtr NovaPathPlanner::create_compat_node_from_lifecycle(
    const rclcpp_lifecycle::LifecycleNode::SharedPtr &lifecycle_node) {
    RCLCPP_INFO(get_node()->get_logger(), "Creating compatability node");

    auto options = rclcpp::NodeOptions()
      .context(lifecycle_node->get_node_base_interface()->get_context());

    std::string compat_node_name = std::string(lifecycle_node->get_name()) + "_kinematics_compat";

    return std::make_shared<rclcpp::Node>(
      compat_node_name,
      lifecycle_node->get_namespace(),
      options);
  }

  std::string NovaPathPlanner::joint_to_command_interface_name(const std::string& joint_name) const {
    // If chained_controller_name is non-empty, prepend it plus "/"
    // Example result: "nova_arm_controller/J1/position"
    const auto prefix = params_.chained_controller_name.empty() ? "" : params_.chained_controller_name + "/";
    return prefix + joint_name + "/" + hardware_interface::HW_IF_POSITION;
  }

  bool NovaPathPlanner::check_path_for_self_intersection(const std::vector<double> &seed_state,
                                                         const std::vector<double> &target_positions) {
    // Find the largest difference between values in seed_state and target_positions
    auto largest_displacement = 0;
    for (size_t i = 0; i < seed_state.size(); i++) {
      auto displacement = abs(target_positions[i] - seed_state[i]);

      if (displacement > largest_displacement) {
        largest_displacement = displacement;
      }
    }

    auto iterations = static_cast<int>(ceil(fmod(largest_displacement, params_.self_intersection_max_step_size)));

    // Step N=(iterations-1) times from seed_state to target_positions, checking for self intersections.
    // Excludes checking seed_state. target_positions is checked after this block.
    std::vector<double> intermediate_positions(seed_state.size());
    for (int i = 1; i < iterations; i++) {
      auto interpolator = static_cast<double>(i) / iterations;
      auto one_minus_interpolator = 1 - interpolator;

      for (size_t j = 0; j < intermediate_positions.size(); j++) {
        // Lerp between seed_state and target_positions
        intermediate_positions[j] = seed_state[j] * one_minus_interpolator + target_positions[j] * interpolator;
      }

      if (check_pose_for_self_intersection(intermediate_positions)) {
        return true;
      }
    }

    // Always check the target position
    return check_pose_for_self_intersection(target_positions);
  }

  bool NovaPathPlanner::check_pose_for_self_intersection(const std::vector<double> &joint_positions) {
    // TODO: Implement max joint distance moved per check, and do multiple iterations for changes in joint values that
    //  exceed that min step size.
    auto logger = get_node()->get_logger();

    for (auto& joint_position : joint_positions) {
      if (std::isnan(joint_position) || std::isinf(joint_position)) {
        RCLCPP_ERROR(logger, "Received NaN or Inf position for joint in self intersection check.");
        return true;
      }
    }

    // Create state matching joint_positions
    moveit::core::RobotState& state = planning_scene_->getCurrentStateNonConst();
    state.setToDefaultValues();
    state.setJointGroupPositions(joint_group_name_, joint_positions);
    state.update();

    // Just in case, also try update mimic joints
    for (const auto* joint_model : robot_model_->getMimicJointModels()) {
      if (!joint_model)
        continue;

      const auto* source_joint = joint_model->getMimic();
      if (!source_joint)
        continue;

      const auto* position_ptr = state.getJointPositions(source_joint);
      if (!position_ptr)
        continue;

      const auto position = joint_model->getMimicFactor() * (*position_ptr) + joint_model->getMimicOffset();
      state.setVariablePosition(joint_model->getName(), position);
    }
    state.update();

    collision_detection::CollisionRequest req;
    collision_detection::CollisionResult res;

    // TODO: Only do this when requested
    req.contacts = true;           // Request contact info
    req.max_contacts = 10;         // Limit contact count

    planning_scene_->checkSelfCollision(req, res, state);

    if (res.collision) {
      RCLCPP_WARN(logger, "Self intersection detected for pose. Found collisions between:");
      for (const auto& contact : res.contacts) {
        RCLCPP_WARN(logger, "  - \"%s\" and \"%s\"", contact.first.first.c_str(), contact.first.second.c_str());
      }
      return true;
    }

    return false;
  }

  void NovaPathPlanner::generate_allowed_collision_matrix() {
    auto logger = get_node()->get_logger();
    RCLCPP_DEBUG(logger, "Generating allowed collision matrix for self intersection checks. "
                        "Ignoring collisions between:");
    auto acm = planning_scene_->getAllowedCollisionMatrix();

    // Get the current state
    moveit::core::RobotState& state = planning_scene_->getCurrentStateNonConst();
    state.setToDefaultValues();
    state.update();

    // Set up request/result
    collision_detection::CollisionRequest req;
    collision_detection::CollisionResult res;
    req.contacts = true;      // We get contact info to generate the allowed collision matrix from
    req.max_contacts = 1024;  // This was chosen arbitrarily as some large number

    // Perform self-collision check
    planning_scene_->checkSelfCollision(req, res, state);

    // Add all colliding pairs to the ACM
    for (const auto &contact_pair : res.contacts)
    {
      RCLCPP_DEBUG(logger, "  - \"%s\" and \"%s\"",
                  contact_pair.first.first.c_str(), contact_pair.first.second.c_str());
      const auto &link1 = contact_pair.first.first;
      const auto &link2 = contact_pair.first.second;
      acm.setEntry(link1, link2, true);  // Mark this pair as allowed to collide
    }

    planning_scene_->setAllowedCollisionMatrix(acm);
  }

  rclcpp_action::GoalResponse NovaPathPlanner::handle_action_goal(const rclcpp_action::GoalUUID &uuid,
                                                                  std::shared_ptr<const ArmPlanPath::Goal> goal) {
    auto logger = get_node()->get_logger();
    if (is_path_being_executed_) {
      RCLCPP_ERROR(logger, "Goal rejected because a path is already being executed.");
      return rclcpp_action::GoalResponse::ACCEPT_AND_DEFER;
    }

    RCLCPP_INFO(logger, "Received and accepted goal request.");
    (void)uuid;

    return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
  }

  void NovaPathPlanner::handle_action_accepted(const std::shared_ptr<GoalHandleArmPlanPath>& goal_handle) {
    // this needs to return quickly to avoid blocking the executor, so spin up a new thread
    std::thread{std::bind(&nova_path_planner::NovaPathPlanner::execute_action, this, _1), goal_handle}.detach();
  }

  rclcpp_action::CancelResponse NovaPathPlanner::handle_action_cancelled(const std::shared_ptr<GoalHandleArmPlanPath>& goal_handle) {
    auto logger = get_node()->get_logger();
    RCLCPP_INFO(logger, "Received request to cancel goal");
    (void)goal_handle;
    return rclcpp_action::CancelResponse::ACCEPT;
  }

  void NovaPathPlanner::execute_action(const std::shared_ptr<GoalHandleArmPlanPath> goal_handle) {
    auto logger = get_node()->get_logger();
    RCLCPP_INFO(logger, "Executing goal");
    const auto goal = goal_handle->get_goal();
    auto result = std::make_shared<ArmPlanPath::Result>();

    // Set up for path planning
    Eigen::Isometry3d start; // Get current pose

    Eigen::Isometry3d end;
    Eigen::fromMsg(goal->pose, end);

    // Handles for a cubic bezier spline
    const Eigen::Isometry3d& handle0 = start;
    const Eigen::Isometry3d handle1 = end;

    double execution_time = 1.0;
    double frequency = get_update_rate();

    // Calculate number of points
    int pose_count_minus_one = static_cast<int>(std::floor(execution_time / frequency));
    int pose_count = pose_count_minus_one + 1;

    auto path = std::make_shared<std::queue<std::vector<double>>>();

    // Plan the path
    // This should be a loop that frequently releases to the scheduler
    auto last_joint_pose = get_state_pos_values();

    for (int i = 0; i < pose_count; i++) {
      const auto t = static_cast<double>(i) / pose_count_minus_one;

      // Calculate as cubic bezier curve
      Eigen::Isometry3d pose = lerp3(start, handle0, handle1, end, t);

      std::vector<double> solution;
      geometry_msgs::msg::Pose pose_msg = tf2::toMsg(pose);
      moveit_msgs::msg::MoveItErrorCodes error_codes;

      auto ik_result = kinematics_solver_->searchPositionIK(pose_msg, last_joint_pose, params_.kinematics_solver_timeout, solution, error_codes);

      if (!ik_result) {
        RCLCPP_FATAL(logger, "Failed to find solution to inverse kinematics at t=%f: error code %d (\"%s\"). %s",
                             t, error_codes.val, moveit::core::errorCodeToString(error_codes).c_str(),
                             error_codes.message.c_str());
        is_path_being_executed_ = false;
        result->success = false;
        goal_handle->succeed(result);
        return;
      }

      if (check_path_for_self_intersection(last_joint_pose, solution)) {
        RCLCPP_FATAL(logger, "Inverse Kinematics solution self intersects for t=%f!", t);
        is_path_being_executed_ = false;
        result->success = false;
        goal_handle->succeed(result);
        return;
      }

      last_joint_pose = solution;
      path->push(solution);

      // TODO: Verify this actually permits other threads
      std::this_thread::yield();
    }

    // Give to the realtime thread to be executed physically by the control loop
    { // mutex owning block
      std::unique_lock<std::mutex> lock(path_mutex_);
      path_ptr_.set(path);
    } // Release mutex

    // Wait for the path to be executed
    rclcpp::Rate wait_loop_rate(10);
    bool executing_path = true;  // Becomes false when the queue in path_ptr_ becomes empty

    while (executing_path) {
      if (goal_handle->is_canceling()) {
        clear_path_ptr();
        goal_handle->canceled(result);
        RCLCPP_INFO(logger, "Goal canceled");
        return;
      }

      // TODO: check this is actually what we want to do in this case. Functionally equivalent to example action server.
      if (!rclcpp::ok()) {
        clear_path_ptr();
        return;
      }

      { // mutex owning block
        std::unique_lock<std::mutex> lock(path_mutex_);

        std::shared_ptr<std::queue<std::vector<double>>> current_path;
        path_ptr_.get(current_path);

        executing_path = current_path && !current_path->empty();
      } // Release mutex

      // TODO: Verify this actually permits other threads
      std::this_thread::yield();
    }

    clear_path_ptr();
    goal_handle->succeed(result);
    RCLCPP_INFO(logger, "Goal succeeded");
  }

  void NovaPathPlanner::clear_path_ptr() {
    {
      std::unique_lock<std::mutex> lock(path_mutex_);
      path_ptr_.set(nullptr);
    }
    is_path_being_executed_ = false;
  }

  inline Eigen::Vector3d NovaPathPlanner::lerp(const Vector3d& a, const Vector3d& b, const double &t) {
    return (1-t)*a + t*b;
  }

  inline Eigen::Vector3d NovaPathPlanner::lerp2(const Vector3d& a, const Vector3d& b, const Vector3d& c, const double &t) {
    return lerp(lerp(a,b, t), lerp(b,c, t), t);
  }

  inline Eigen::Vector3d NovaPathPlanner::lerp3(const Vector3d& a, const Vector3d& b, const Vector3d& c, const Vector3d& d, const double &t) {
    return lerp(lerp2(a,b,c, t), lerp2(b,c,d, t), t);
  }

  inline Eigen::Quaterniond NovaPathPlanner::slerp(const Eigen::Quaterniond& a, const Eigen::Quaterniond& b, const double &t) {
    return a.slerp(t, b);
  }

  inline Eigen::Quaterniond NovaPathPlanner::slerp2(const Eigen::Quaterniond& a, const Eigen::Quaterniond& b, const Eigen::Quaterniond& c, const double &t) {
    return slerp(slerp(a,b, t), slerp(b,c, t), t);
  }

  inline Eigen::Quaterniond
  NovaPathPlanner::slerp3(const Eigen::Quaterniond &a, const Eigen::Quaterniond &b, const Eigen::Quaterniond &c, const Eigen::Quaterniond &d,
                          const double &t) {
    return slerp(slerp2(a,b,c, t), slerp2(b,c,d, t), t);
  }

  inline Eigen::Isometry3d
  NovaPathPlanner::lerp3(Eigen::Isometry3d a, Eigen::Isometry3d b, Eigen::Isometry3d c, Eigen::Isometry3d d, double t) {
    const Vector3d translation = lerp3(a.translation(), b.translation(), c.translation(), d.translation(), t);
    const Eigen::Quaterniond rotation = slerp3(
      Eigen::Quaterniond(a.rotation()),
      Eigen::Quaterniond(b.rotation()),
      Eigen::Quaterniond(c.rotation()),
      Eigen::Quaterniond(d.rotation()),
      t);

    Eigen::Isometry3d result = Eigen::Isometry3d::Identity();
    result.linear() = rotation.toRotationMatrix();
    result.translation() = translation;

    return result;
  }

} // namespace nova_path_planner

#include "class_loader/register_macro.hpp"

CLASS_LOADER_REGISTER_CLASS(
    nova_path_planner::NovaPathPlanner, controller_interface::ControllerInterface)
