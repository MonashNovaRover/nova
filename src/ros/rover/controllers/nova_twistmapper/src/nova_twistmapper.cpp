/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	  nova_twistmapper
AUTHOR:     Bailey Chessum
EDITED BY:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "nova_twistmapper/nova_twistmapper.hpp"
#include "lifecycle_msgs/msg/state.hpp"
#include "rclcpp/logging.hpp"
#include "tf2_geometry_msgs/tf2_geometry_msgs/tf2_geometry_msgs.hpp"
#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "tf2_eigen/tf2_eigen.hpp"
#include <urdf_parser/urdf_parser.h>
#include <srdfdom/model.h>
#include <pluginlib/class_loader.hpp>
#include <moveit/kinematics_base/kinematics_base.h>
#include <stdexcept>
#include <ranges>


namespace
{
  constexpr auto DEFAULT_INPUT_TOPIC_END_EFFECTOR_TWIST = "/arm_ik_twist_stamped"; // TODO: changeme
  constexpr auto ENDEFFECTOR_KINEMATICS_FRAME = "endeffector_kinematics";
  constexpr auto KINEMATICS_ORIGIN_FRAME = "arm_kinematics_origin";
  constexpr auto TWISTMAPPER_TARGET_FRAME = "arm_twistmapper_target";
} // namespace

using std::placeholders::_1;

namespace nova_twistmapper
{
  using namespace std::chrono_literals;
  using controller_interface::interface_configuration_type;
  using controller_interface::InterfaceConfiguration;
  using lifecycle_msgs::msg::State;


  NovaTwistmapper::NovaTwistmapper() : controller_interface::ControllerInterface() {}

  controller_interface::CallbackReturn NovaTwistmapper::on_init()
  {
    try
    {
      // Create the parameter listener and get the parameters
      param_listener_ = std::make_shared<ParamListener>(get_node());
      params_ = param_listener_->get_params();
    }
    catch (const std::exception &e)
    {
      fprintf(stderr, "Exception thrown during init stage with message: %s \n", e.what());
      return controller_interface::CallbackReturn::ERROR;
    }

    kinematics_solver_loader_ = std::make_unique<pluginlib::ClassLoader<kinematics::KinematicsBase>>(
      "moveit_core", "kinematics::KinematicsBase");

    auto logger = get_node()->get_logger();
    auto qos = rclcpp::QoS(rclcpp::KeepLast(1)).transient_local().reliable();
    robot_description_sub_ = get_node()->create_subscription<std_msgs::msg::String>(
      "/robot_description",
      qos,
      [this, logger](const std::shared_ptr<std_msgs::msg::String> msg) -> void
      {
        RCLCPP_INFO_ONCE(logger, "/robot_description received!");
        received_robot_description_ptr_.set(std::move(msg));
      });

    return controller_interface::CallbackReturn::SUCCESS;
  }

  InterfaceConfiguration NovaTwistmapper::command_interface_configuration() const
  {
    std::vector<std::string> conf_names;

    for (const auto &joint : params_.joint_names)
    {
      conf_names.push_back(joint_to_command_interface_name(joint));
    }

    return {interface_configuration_type::INDIVIDUAL, conf_names};
  }

  InterfaceConfiguration NovaTwistmapper::state_interface_configuration() const
  {
    std::vector<std::string> conf_names;

    for (const auto &joint_name: params_.joint_names) {
      conf_names.push_back(joint_name + "/" + hardware_interface::HW_IF_POSITION);
    }

    return {interface_configuration_type::INDIVIDUAL, conf_names};
  }

  void NovaTwistmapper::update_twistmapper_pose(const rclcpp::Time &time, const rclcpp::Duration &period) {
    std::shared_ptr<geometry_msgs::msg::TwistStamped> twist_stamped;
    received_twist_stamped_ptr_.get(twist_stamped);

    if (twist_stamped == nullptr) {
      RCLCPP_WARN(get_node()->get_logger(), "Haven't yet received a TwistStamped message to use for the twistmapper.");
      return;
    }

    const auto& twist = twist_stamped->twist;

    tf2::Vector3 tf2_twist_angular;
    tf2::fromMsg(twist.angular, tf2_twist_angular);

    twistmapper_pose_rpy_ = twistmapper_pose_rpy_ + tf2_twist_angular * period.seconds();
    // Create rotation matrix from twist.angular as XYZ euler
    auto rotation_matrix = tf2::Matrix3x3();
    rotation_matrix.setRPY(twistmapper_pose_rpy_.x(), twistmapper_pose_rpy_.y(), twistmapper_pose_rpy_.z());

    const auto linear = tf2::Vector3(
      twist.linear.x * period.seconds(),
      twist.linear.y * period.seconds(),
      twist.linear.z * period.seconds());

    // TODO: Test this actually works as expected.
    // TODO: Add a tf buffer and make use of the header.frame_id from the twist_stamped to allow IK to be done relative to anything. Then Teleop would just need to use the end effector frame by default
    // TODO: If using tf like this, also have the twistmapper broadcast the target frame, so it can reference itself.
    twistmapper_pose_.setOrigin(linear + twistmapper_pose_.getOrigin());
    twistmapper_pose_.setBasis(rotation_matrix);
  }

  controller_interface::return_type NovaTwistmapper::update(const rclcpp::Time &time, const rclcpp::Duration &period)
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

    auto old_pose = twistmapper_pose_;

    // Get a new pose
    update_twistmapper_pose(time, period);
    publish_to_tf2(time);

    // Do IK to find the joint values for that pose
    std::vector<double> joint_state_values = get_state_pos_values();
    std::vector<double> solution;
    geometry_msgs::msg::Pose pose = tf2::toMsg(tf2::transformToEigen(tf2::toMsg(twistmapper_pose_)));
    moveit_msgs::msg::MoveItErrorCodes error_codes;

    auto result = kinematics_solver_->getPositionIK(pose, joint_state_values, solution, error_codes);
    if (!result) {
      twistmapper_pose_ = old_pose;
      RCLCPP_WARN_THROTTLE(logger, *get_node()->get_clock(), 500, "Failed to find solution to inverse kinematics.");
      return controller_interface::return_type::OK;
    }

    if (check_collisions_for_pose(solution)) {
      twistmapper_pose_ = old_pose;
      RCLCPP_WARN_THROTTLE(logger, *get_node()->get_clock(), 500, "Inverse Kinematics solution self intersects!");
      return controller_interface::return_type::OK;
    }

    // Apply solution to command interfaces
    for (size_t i = 0; i < solution.size(); i++) {
      registered_joint_handles_[i].command.get().set_value(solution[i]);
    }

    return controller_interface::return_type::OK;
  }

  controller_interface::CallbackReturn NovaTwistmapper::on_configure(const rclcpp_lifecycle::State&)
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

    twistmapper_pose_tf_broadcaster_ = std::make_shared<tf2_ros::TransformBroadcaster>(tf2_ros::TransformBroadcaster(*get_node()));
    tf_buffer_ = std::make_unique<tf2_ros::Buffer>(get_node()->get_clock());
    tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

    twist_stamped_sub_ = get_node()->create_subscription<geometry_msgs::msg::TwistStamped>(
      DEFAULT_INPUT_TOPIC_END_EFFECTOR_TWIST,
      rclcpp::SystemDefaultsQoS(),
      [this, logger](const std::shared_ptr<geometry_msgs::msg::TwistStamped> msg) -> void
      {
        RCLCPP_INFO_ONCE(logger, "Twist message received.");

        if (!subscriber_is_active_)
        {
          RCLCPP_WARN_ONCE(
            logger, "Can't accept new commands. subscriber is inactive");
          return;
        }
        if ((msg->header.stamp.sec == 0) && (msg->header.stamp.nanosec == 0))
        {
          RCLCPP_WARN_ONCE(
            logger,
            "Received message with zero timestamp, setting it to current "
            "time, this message will only be shown once");
          msg->header.stamp = get_node()->get_clock()->now();
        }

        received_twist_stamped_ptr_.set(std::move(msg));
      });
    subscriber_is_active_ = true;

    RCLCPP_INFO(logger, "Created twist stamped subscription");

    // Parse URDF
    std::string urdf_str = params_.robot_description;

    if (urdf_str.empty()) {
      RCLCPP_WARN(get_node()->get_logger(), "No URDF was provided in robot_description! Attempting to load from topic.");
      if (!get_urdf_from_topic(0.2, urdf_str)) {
        RCLCPP_ERROR(get_node()->get_logger(), "Failed to parse the given robot_description parameter");
        return CallbackReturn::FAILURE;
      }
    }
    else {
      RCLCPP_INFO(logger, "Found URDF string from robot_description parameter");
    }

    urdf_model_ = urdf::parseURDF(urdf_str);
    if (!urdf_model_) {
      RCLCPP_ERROR(get_node()->get_logger(),
                   "Failed to parse the given robot_description URDF string \"%s\"", urdf_str.c_str());
      return CallbackReturn::FAILURE;
    }

    // Create an SRDF with a joint group for params_.joint_names
    srdf_model_ = std::make_shared<srdf::Model>();
    joint_group_name_ = std::basic_string(get_node()->get_name()) + "_joints";
    auto srdf_string = construct_srdf_fallback_string(urdf_model_, joint_group_name_);
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
    const std::basic_string<char> base_frame = KINEMATICS_ORIGIN_FRAME;
    const std::vector<std::basic_string<char>> tip_frames {
      ENDEFFECTOR_KINEMATICS_FRAME
    };
    // We currently don't use this. Reasonable values are in [0.01, 0.1] rads, and KDL uses 0.1 rads by default.
    double search_discretization = 0.1;

    kinematics_solver_->initialize(kinematics_compat_node_, *robot_model_.get(), joint_group_name_, base_frame, tip_frames,
                                   search_discretization);

    previous_update_timestamp_ = get_node()->get_clock()->now();
    return controller_interface::CallbackReturn::SUCCESS;
  }

  std::string NovaTwistmapper::construct_srdf_fallback_string(const urdf::ModelInterfaceSharedPtr &urdf_model,
                                                              std::string joint_group_name) {
    // TODO: Allow an SRDF to be provided
    std::ostringstream srdf_stream;
    srdf_stream << "<robot name=\"" << urdf_model->getName() << "\">\n";
    srdf_stream << "  <group name=\"" << joint_group_name << "\">\n";
    for (const auto& joint : params_.joint_names)
      srdf_stream << "    <joint name=\"" << joint << "\"/>\n";
    srdf_stream << "  </group>\n";
    srdf_stream << "</robot>\n";
    auto srdf_string = srdf_stream.str();
    return srdf_string;
  }

  bool NovaTwistmapper::get_urdf_from_topic(double timeout_sec, std::string &urdf_string)
  {
    std::shared_ptr<std_msgs::msg::String> robot_description_msg;
    auto start = std::chrono::steady_clock::now();

    while (std::chrono::steady_clock::now() - start < std::chrono::duration<double>(timeout_sec)) {
      received_robot_description_ptr_.get(robot_description_msg);
      if (robot_description_msg) {
        urdf_string = robot_description_msg->data;
        return true;
      }

      // Sleep briefly to avoid spinning hot
      std::this_thread::sleep_for(10ms);
    }

    return false;
  }

  controller_interface::CallbackReturn NovaTwistmapper::on_activate(const rclcpp_lifecycle::State&)
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

    auto joint_values = get_state_pos_values();
    std::vector<geometry_msgs::msg::Pose> poses;

    auto result = kinematics_solver_->getPositionFK({ENDEFFECTOR_KINEMATICS_FRAME}, joint_values, poses);

    if (!result) {
      RCLCPP_ERROR(get_node()->get_logger(), "Failed to do forward kinematics to find the end effector's initial pose");
      return CallbackReturn::ERROR;
    }
    if (poses.empty()) {
      RCLCPP_ERROR(get_node()->get_logger(), "No poses returned from forward kinematics!");
      return CallbackReturn::ERROR;
    }

    // Store result to twistmapper_pose_
    tf2::fromMsg(poses[0], twistmapper_pose_);

    // Set twistmapper_pose_rpy_ to match
    double roll, pitch, yaw;
    twistmapper_pose_.getBasis().getRPY(roll, pitch, yaw);
    twistmapper_pose_rpy_.setX(roll);
    twistmapper_pose_rpy_.setY(pitch);
    twistmapper_pose_rpy_.setZ(yaw);

    // TODO: Potentially set initial command interface values from state interface

    RCLCPP_INFO(get_node()->get_logger(), "Initial twistmapper pose set from forward kinematics.");
    return controller_interface::CallbackReturn::SUCCESS;
  }

  controller_interface::CallbackReturn NovaTwistmapper::on_deactivate(const rclcpp_lifecycle::State&)
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

  controller_interface::CallbackReturn NovaTwistmapper::on_cleanup(const rclcpp_lifecycle::State&)
  {
    RCLCPP_INFO(get_node()->get_logger(), "On Cleanup");

    if (!reset())
    {
      return controller_interface::CallbackReturn::ERROR;
    }

    return controller_interface::CallbackReturn::SUCCESS;
  }

  controller_interface::CallbackReturn NovaTwistmapper::on_error(const rclcpp_lifecycle::State&)
  {
    RCLCPP_INFO(get_node()->get_logger(), "On Error");
    if (!reset())
    {
      return controller_interface::CallbackReturn::ERROR;
    }
    return controller_interface::CallbackReturn::SUCCESS;
  }

  bool NovaTwistmapper::reset()
  {
    RCLCPP_INFO(get_node()->get_logger(), "TEMP: Resetting.");

    is_halted = false;

    // Reset pointers
    kinematics_solver_.reset();
    robot_model_.reset();
    urdf_model_.reset();
    srdf_model_.reset();
    kinematics_compat_node_.reset();

    twistmapper_pose_tf_broadcaster_.reset();
    tf_buffer_.reset();
    tf_listener_.reset();

    // Reset subscriptions
    twist_stamped_sub_.reset();
    subscriber_is_active_ = false;

    return true;
  }

  controller_interface::CallbackReturn NovaTwistmapper::on_shutdown(const rclcpp_lifecycle::State&)
  {
    return controller_interface::CallbackReturn::SUCCESS;
  }

  void NovaTwistmapper::halt()
  {
    // Set twistmapper velocities to 0
    std::shared_ptr<geometry_msgs::msg::TwistStamped> twist_stamped;
    received_twist_stamped_ptr_.get(twist_stamped);

    twist_stamped->twist.linear.x = 0;
    twist_stamped->twist.linear.y = 0;
    twist_stamped->twist.linear.z = 0;

    twist_stamped->twist.angular.x = 0;
    twist_stamped->twist.angular.y = 0;
    twist_stamped->twist.angular.z = 0;
  }

  void NovaTwistmapper::publish_to_tf2(const rclcpp::Time &time) {
    // Publish twistmapper pose to tf2
    geometry_msgs::msg::TransformStamped transform_stamped;
    transform_stamped.transform = toMsg(twistmapper_pose_);
    transform_stamped.header.stamp = time;

    // TODO: Parameterize
    transform_stamped.child_frame_id = TWISTMAPPER_TARGET_FRAME;
    transform_stamped.header.frame_id = KINEMATICS_ORIGIN_FRAME;

    RCLCPP_INFO_ONCE(get_node()->get_logger(), "Broadcasting twistmapper pose as '%s', child of '%s'.",
                     transform_stamped.child_frame_id.c_str(),
                     transform_stamped.header.frame_id.c_str());

    twistmapper_pose_tf_broadcaster_->sendTransform(transform_stamped);
  }

  controller_interface::CallbackReturn NovaTwistmapper::configure_joints() {
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

  std::vector<double> NovaTwistmapper::get_state_pos_values() {
    std::vector<double> joint_values;

    joint_values.reserve(registered_joint_handles_.size());
    for (auto& joint_handle : registered_joint_handles_)
      joint_values.emplace_back(joint_handle.state_pos.get().get_value());
    return joint_values;
  }

  rclcpp::Node::SharedPtr NovaTwistmapper::create_compat_node_from_lifecycle(
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

  std::string NovaTwistmapper::joint_to_command_interface_name(const std::string& joint_name) const {
    // If chained_controller_name is non-empty, prepend it plus "/"
    // Example result: "nova_arm_controller/J1/position"
    const auto prefix = params_.chained_controller_name.empty() ? "" : params_.chained_controller_name + "/";
    return prefix + joint_name + "/" + hardware_interface::HW_IF_POSITION;
  }

  bool NovaTwistmapper::check_collisions_for_pose(const std::vector<double> &joint_positions) {
    // TODO: Implement max joint distance moved per check, and do multiple iterations for changes in joint values that
    //  exceed that min step size.
    auto logger = get_node()->get_logger();

    for (auto& joint_position : joint_positions) {
      if (std::isnan(joint_position) || std::isinf(joint_position)) {
        RCLCPP_ERROR(logger, "Received NaN or Inf position for joint in self intersection check.", joint_position);
        return false;
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
      RCLCPP_WARN(logger, "Self intersection detected for target pose!");

      for (auto contact : res.contacts) {
        RCLCPP_WARN(logger, "Self intersection check found contact between \"%s\" and \"%s\"",
                    contact.first.first.c_str(), contact.first.second.c_str());
      }

      return true;
    }

    return false;
  }

  void NovaTwistmapper::generate_allowed_collision_matrix() {
    auto logger = get_node()->get_logger();
    RCLCPP_INFO(logger, "Generating allowed collision matrix for self intersection checks...");
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
      RCLCPP_INFO(logger, "Ignoring contact between \"%s\" and \"%s\"",
                  contact_pair.first.first.c_str(), contact_pair.first.second.c_str());
      const auto &link1 = contact_pair.first.first;
      const auto &link2 = contact_pair.first.second;
      acm.setEntry(link1, link2, true);  // Mark this pair as allowed to collide
    }

    planning_scene_->setAllowedCollisionMatrix(acm);
  }
} // namespace nova_twistmapper

#include "class_loader/register_macro.hpp"

CLASS_LOADER_REGISTER_CLASS(
    nova_twistmapper::NovaTwistmapper, controller_interface::ControllerInterface)
