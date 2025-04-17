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

namespace
{
  constexpr auto DEFAULT_INPUT_TOPIC_END_EFFECTOR_TWIST = "/arm_ik_twist_stamped"; // TODO: changeme
  constexpr auto ENDEFFECTOR_KINEMATICS_FRAME = "endeffector_kinematics";
  constexpr auto KINEMATICS_ORIGIN_FRAME = "arm_kinematics_origin";
  constexpr auto TWISTMAPPER_TARGET_FRAME = "arm_twistmapper_target";

  constexpr std::array<const char* const, 7> POSE_COMPONENTS = {
    "x",
    "y",
    "z",
    "qx",
    "qy",
    "qz",
    "qw",
  };
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

    return controller_interface::CallbackReturn::SUCCESS;
  }

  InterfaceConfiguration NovaTwistmapper::command_interface_configuration() const
  {
    std::vector<std::string> conf_names;
    for (const auto &component_name : POSE_COMPONENTS)
    {
      conf_names.push_back(pose_component_to_command_interface_name(component_name));
    }

    return {interface_configuration_type::INDIVIDUAL, conf_names};
  }

  InterfaceConfiguration NovaTwistmapper::state_interface_configuration() const
  {
    std::vector<std::string> conf_names;

    return {interface_configuration_type::INDIVIDUAL, conf_names};
  }

  void NovaTwistmapper::update_twistmapper_pose(const rclcpp::Time &time, const rclcpp::Duration &period) {
    std::shared_ptr<geometry_msgs::msg::TwistStamped> twist_stamped;
    received_twist_stamped_ptr.get(twist_stamped);

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

    if (!pose_handle.has_value())
      return controller_interface::return_type::ERROR;

    update_twistmapper_pose(time, period);
    publish_to_tf2(time);
    pose_handle.value().set_value(twistmapper_pose_);

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

    twist_stamped_sub = get_node()->create_subscription<geometry_msgs::msg::TwistStamped>(
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

        received_twist_stamped_ptr.set(std::move(msg));
      });
    subscriber_is_active_ = true;

    RCLCPP_INFO(logger, "Created twist stamped subscription");

    previous_update_timestamp_ = get_node()->get_clock()->now();
    return controller_interface::CallbackReturn::SUCCESS;
  }

  std::string NovaTwistmapper::get_urdf_from_topic(const std::string &topic_name, double timeout_sec)
  {
    std::promise<std::string> urdf_promise;
    auto future = urdf_promise.get_future();

    auto sub = get_node().get()->create_subscription<std_msgs::msg::String>(
      topic_name, 1,
      [&urdf_promise](const std_msgs::msg::String::SharedPtr msg) {
        urdf_promise.set_value(msg->data);
      });

    // Spin until we get the message or timeout
    auto start = std::chrono::steady_clock::now();

    while (rclcpp::ok() && future.wait_for(std::chrono::milliseconds(100)) != std::future_status::ready)
    {
      if ((std::chrono::steady_clock::now() - start) > std::chrono::duration<double>(timeout_sec))
      {
        throw std::runtime_error("Timeout waiting for /robot_description topic");
      }
    }

    return future.get();
  }

  controller_interface::CallbackReturn NovaTwistmapper::on_activate(const rclcpp_lifecycle::State&)
  {
    RCLCPP_INFO(get_node()->get_logger(), "On activate");

    // Set up joint state interfaces
    const auto joints_result = configure_joints();
    if (joints_result == controller_interface::CallbackReturn::ERROR)
    {
      RCLCPP_ERROR(get_node()->get_logger(), "Some joint interfaces are non existent");
      return controller_interface::CallbackReturn::ERROR;
    }

    const auto prefix = params_.chained_controller_name.empty() ? ""
      : params_.chained_controller_name + "/";

    // Configure command interfaces for the pose
    try {
      pose_handle = PoseHandle::pose_handle_from_command_interfaces(command_interfaces_, prefix);
    } catch (const std::runtime_error& e) {
      RCLCPP_ERROR(get_node()->get_logger(), "%s. Do the given chainable controller name and interfaces exist?",
                   e.what());
      return CallbackReturn::FAILURE;
    }

    is_halted = false;
    subscriber_is_active_ = true;

    std::string urdf_str;

    // Parse URDF
    try {
      urdf_str = get_urdf_from_topic();
    } catch (const std::runtime_error& e) {
      RCLCPP_ERROR(get_node()->get_logger(), "Failed to get the rover URDF.", e.what());
      return CallbackReturn::FAILURE;
    }
    auto urdf_model = urdf::parseURDF(urdf_str);
    if (!urdf_model) {
      throw std::runtime_error("Failed to parse URDF");
    }

    // Finally create the robot model
    robot_model_ = std::make_shared<moveit::core::RobotModel>(urdf_model, nullptr);

    moveit::core::RobotState robot_state(robot_model_);
    robot_state.setToDefaultValues();  // Optional: start from known state

    // Set position states of the rover to the initial state interface values
    for (auto& joint : registered_joint_handles_) {
      robot_state.setVariablePosition(joint.name, joint.state_pos.get().get_value());
    }
    robot_state.update();  // Compute transforms for above applied joint positions

    bool frame_found = false;
    auto endeffector_global_transform = robot_state.getFrameTransform(ENDEFFECTOR_KINEMATICS_FRAME, &frame_found);
    if (!frame_found) {
      RCLCPP_ERROR(get_node()->get_logger(), "Failed to get Transform for frame \'%s\'", ENDEFFECTOR_KINEMATICS_FRAME);
      return controller_interface::CallbackReturn::ERROR;
    }

    auto kinematics_origin_transform = robot_state.getFrameTransform(KINEMATICS_ORIGIN_FRAME, &frame_found);
    if (!frame_found) {
      RCLCPP_ERROR(get_node()->get_logger(), "Failed to get Transform for frame \'%s\'", KINEMATICS_ORIGIN_FRAME);
      return controller_interface::CallbackReturn::ERROR;
    }

    auto endeffector_transform = kinematics_origin_transform.inverse() * endeffector_global_transform;

    convert(endeffector_transform, twistmapper_pose_);


    /*try {
      auto tf_transform = tf_buffer_->lookupTransform(
        KINEMATICS_ORIGIN_FRAME, ENDEFFECTOR_KINEMATICS_FRAME, tf2::TimePointZero);
      tf2::fromMsg(tf_transform.transform, twistmapper_pose_);

      double roll, pitch, yaw;
      twistmapper_pose_.getBasis().getRPY(roll, pitch, yaw);

      twistmapper_pose_rpy_.setX(roll);
      twistmapper_pose_rpy_.setY(pitch);
      twistmapper_pose_rpy_.setZ(yaw);
    } catch (const tf2::TransformException & ex) {
      RCLCPP_INFO(
        get_node()->get_logger(), "Could not transform %s to %s: %s",
        ENDEFFECTOR_KINEMATICS_FRAME, KINEMATICS_ORIGIN_FRAME, ex.what());
      return controller_interface::CallbackReturn::FAILURE;
    }*/

    // Set the initial command interface values
    pose_handle->set_value(twistmapper_pose_);

    RCLCPP_DEBUG(get_node()->get_logger(), "Subscriber and publisher are now active.");
    return controller_interface::CallbackReturn::SUCCESS;
  }

  controller_interface::CallbackReturn NovaTwistmapper::on_deactivate(const rclcpp_lifecycle::State&)
  {
    subscriber_is_active_ = false;
    if (!is_halted)
    {
      halt();
      is_halted = true;
    }

    // Clean up command interfaces
    pose_handle.reset();

    return controller_interface::CallbackReturn::SUCCESS;
  }

  controller_interface::CallbackReturn NovaTwistmapper::on_cleanup(const rclcpp_lifecycle::State&)
  {
    if (!reset())
    {
      return controller_interface::CallbackReturn::ERROR;
    }

    return controller_interface::CallbackReturn::SUCCESS;
  }

  controller_interface::CallbackReturn NovaTwistmapper::on_error(const rclcpp_lifecycle::State&)
  {
    if (!reset())
    {
      return controller_interface::CallbackReturn::ERROR;
    }
    return controller_interface::CallbackReturn::SUCCESS;
  }

  bool NovaTwistmapper::reset()
  {
    // release the old queue
    subscriber_is_active_ = false;

    is_halted = false;
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
    received_twist_stamped_ptr.get(twist_stamped);

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
    registered_joint_handles_.reserve(params_.joint_names.size());
    // TODO: pos/vel/etc limits --> Do we need these here if we are hooking into nova_arm_controller? - Bailey
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

      registered_joint_handles_.emplace_back(
        JointHandle{joint_name, std::ref(*state_handle)});
    }

    return controller_interface::CallbackReturn::SUCCESS;

  }

  std::string NovaTwistmapper::pose_component_to_command_interface_name(const std::string &component_name) const {
    // If chained_controller_name is non-empty, prepend it plus "/"
    // Example result: "nova_arm_controller/J1/position"
    const auto prefix = params_.chained_controller_name.empty() ? "" : params_.chained_controller_name + "/";
    return prefix + component_name;
  }
} // namespace nova_twistmapper

#include "class_loader/register_macro.hpp"

CLASS_LOADER_REGISTER_CLASS(
    nova_twistmapper::NovaTwistmapper, controller_interface::ControllerInterface)
