#include <memory>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "nova_ik_controller/nova_ik_controller.hpp"
#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "lifecycle_msgs/msg/state.hpp"
#include "rclcpp/logging.hpp"
#include "tf2_geometry_msgs/tf2_geometry_msgs/tf2_geometry_msgs.hpp"
#include "tf2/LinearMath/Scalar.h"
//#include "tf2_eigen/tf2_eigen/tf2_eigen.hpp"

namespace
{
  constexpr auto DEFAULT_INPUT_TOPIC_END_EFFECTOR_TWIST = "/arm_ik_twist_stamped"; // TODO: changeme
  /// The inverse of the global orientation the kinematics origin frame wants to rotate to for a roll,pitch,yaw of 0,0,0
  const auto ENDEFFECTOR_BASIS_INVERSE = tf2::Matrix3x3(tf2::Quaternion(-0.5, 0.5, -0.5, -0.5));
  constexpr auto ENDEFFECTOR_KINEMATICS_FRAME = "endeffector_kinematics";
  constexpr auto KINEMATICS_ORIGIN_FRAME = "arm_kinematics_origin";
  constexpr auto TWISTMAPPER_TARGET_FRAME = "arm_twistmapper_target";
} // namespace

using std::placeholders::_1;

namespace nova_ik_controller
{
  using namespace std::chrono_literals;
  using controller_interface::interface_configuration_type;
  using controller_interface::InterfaceConfiguration;
  using lifecycle_msgs::msg::State;

  NovaIKController::NovaIKController() : controller_interface::ControllerInterface() {}

  controller_interface::CallbackReturn NovaIKController::on_init()
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

  InterfaceConfiguration NovaIKController::command_interface_configuration() const
  {
    std::vector<std::string> conf_names;
    for (const auto &joint_name : params_.joint_names)
    {
      conf_names.push_back(joint_to_command_interface_name(joint_name));
    }

    return {interface_configuration_type::INDIVIDUAL, conf_names};
  }

  InterfaceConfiguration NovaIKController::state_interface_configuration() const
  {
    std::vector<std::string> conf_names;

    return {interface_configuration_type::INDIVIDUAL, conf_names};
  }

  void NovaIKController::update_twistmapper(const rclcpp::Time &time, const rclcpp::Duration &period) {
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
    /*rotation_matrix.setRPY(
      twist.angular.x * period.seconds(),
      twist.angular.y * period.seconds(),
      twist.angular.z * period.seconds());
      */
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

  controller_interface::return_type NovaIKController::update(
      const rclcpp::Time &time, const rclcpp::Duration &period)
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

    update_twistmapper(time, period);

    // TODO: Retrieve these values from the robot description, rather than specifying them directly
    // TODO: Or even simpler, just put these in params!!!
    constexpr std::array<double, 3> lengths = {0.5, 0.68332, 0.11073};

    auto joint_angles = calculate_ik(twistmapper_pose_, lengths);

    for (unsigned int i = 0; i < joint_angles.size(); i++)
    {
      auto& joint_handle = registered_joint_handles_[i];
      joint_handle.command.get().set_value(joint_angles[i]);
    }

    return controller_interface::return_type::OK;
  }

  controller_interface::CallbackReturn NovaIKController::on_configure(
      const rclcpp_lifecycle::State &)
  {
    auto logger = get_node()->get_logger();
    RCLCPP_INFO(logger, "On configure");

    // update parameters if they have changed
    if (param_listener_->is_old(params_))
    {
      params_ = param_listener_->get_params();
    }
    if (params_.joint_names.empty())
    {
      return controller_interface::CallbackReturn::ERROR;
    }

    // TODO: maybe limit position so arm doesn't collide?
    if (!reset())
    {
      return controller_interface::CallbackReturn::ERROR;
    }

    twistmapper_pose_tf_broadcaster_ = std::make_shared<tf2_ros::TransformBroadcaster>(tf2_ros::TransformBroadcaster(*get_node()));
    tf_buffer_ = std::make_unique<tf2_ros::Buffer>(get_node()->get_clock());
    tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

    // TODO: insert correct subscriber here
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

  controller_interface::CallbackReturn NovaIKController::on_activate(
      const rclcpp_lifecycle::State &)
  {
    RCLCPP_INFO(get_node()->get_logger(), "On activate");
    const auto joints_result = configure_joints(params_.joint_names, registered_joint_handles_);

    if (joints_result == controller_interface::CallbackReturn::ERROR)
    {
      RCLCPP_ERROR(
          get_node()->get_logger(),
          "Some joint interfaces are non existent");
      return controller_interface::CallbackReturn::ERROR;
    }
    is_halted = false;
    subscriber_is_active_ = true;

    try {
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
    }

    // TODO: setup sub and pub
    RCLCPP_DEBUG(get_node()->get_logger(), "Subscriber and publisher are now active.");
    return controller_interface::CallbackReturn::SUCCESS;
  }

  controller_interface::CallbackReturn NovaIKController::on_deactivate(
      const rclcpp_lifecycle::State &)
  {
    subscriber_is_active_ = false;
    if (!is_halted)
    {
      halt();
      is_halted = true;
    }
    registered_joint_handles_.clear();

    return controller_interface::CallbackReturn::SUCCESS;
  }

  controller_interface::CallbackReturn NovaIKController::on_cleanup(
      const rclcpp_lifecycle::State &)
  {
    if (!reset())
    {
      return controller_interface::CallbackReturn::ERROR;
    }

    return controller_interface::CallbackReturn::SUCCESS;
  }

  controller_interface::CallbackReturn NovaIKController::on_error(const rclcpp_lifecycle::State &)
  {
    if (!reset())
    {
      return controller_interface::CallbackReturn::ERROR;
    }
    return controller_interface::CallbackReturn::SUCCESS;
  }

  bool NovaIKController::reset()
  {
    // release the old queue
    subscriber_is_active_ = false;

    is_halted = false;
    return true;
  }

  controller_interface::CallbackReturn NovaIKController::on_shutdown(
      const rclcpp_lifecycle::State &)
  {
    return controller_interface::CallbackReturn::SUCCESS;
  }

  void NovaIKController::halt()
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

  std::string NovaIKController::joint_to_command_interface_name(const std::string& joint_name) const {
    // If chained_controller_name is non-empty, prepend it plus "/"
    // Example result: "nova_arm_controller/J1/position"
    const auto prefix = params_.chained_controller_name.empty() ? "" : params_.chained_controller_name + "/";
    return prefix + joint_name + "/" + hardware_interface::HW_IF_POSITION;
  }

  controller_interface::CallbackReturn NovaIKController::configure_joints(
      const std::vector<std::string> &joint_names,
      std::vector<JointHandle> &registered_handles)
  {
    auto logger = get_node()->get_logger();
    RCLCPP_INFO(logger, "Configure joints");

    if (joint_names.empty())
    {
      RCLCPP_ERROR(logger, "No joint names specified");
      return controller_interface::CallbackReturn::ERROR;
    }

    // register handles
    registered_handles.reserve(joint_names.size());
    // TODO: pos/vel/etc limits --> Do we need these here if we are hooking into nova_arm_controller? - Bailey
    for (const auto &joint_name : joint_names)
    {
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

      registered_handles.emplace_back(
        JointHandle{joint_name, std::ref(*command_handle)});
    }

    return controller_interface::CallbackReturn::SUCCESS;
  }

  // See Keenan's IK notes
  // TODO: remember to add something for the effector pose
  std::array<double, 6> NovaIKController::calculate_ik(tf2::Transform pose, std::array<double, 3> lengths)
  {
    auto origin = pose.getOrigin();
    auto x = origin.getX();
    auto y = origin.getY();
    auto z = origin.getZ();

    tf2::Matrix3x3 rotated_basis = pose.getBasis() * ENDEFFECTOR_BASIS_INVERSE;

    double l1r = lengths[0];
    double l2r = lengths[1];
    double l3 = lengths[2];

    Eigen::Matrix3d rxyz {
      {rotated_basis[0][0], rotated_basis[0][1], rotated_basis[0][2]},
      {rotated_basis[1][0], rotated_basis[1][1], rotated_basis[1][2]},
      {rotated_basis[2][0], rotated_basis[2][1], rotated_basis[2][2]}
    };  // rxyz orientation matrix

    Eigen::Matrix4d t07r = Eigen::Matrix4d::Zero();
    t07r.topLeftCorner<3,3>() = rxyz;

    t07r(3, 3) = 1;
    t07r(0, 3) = x;
    t07r(1, 3) = y;
    t07r(2, 3) = z;

    Eigen::Matrix4d t67 { {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, l3}, {0, 0, 0, 1} };
    Eigen::Matrix4d t0_wrist = t07r * t67.inverse();

    double wrist_x = t0_wrist(0, 3);
    double wrist_y = t0_wrist(1, 3);
    double wrist_z = t0_wrist(2, 3);

    double j1 = atan2(wrist_y, wrist_x);
    double l = sqrt(pow(wrist_x, 2) + pow(wrist_y, 2));
    double j3a = acos((pow(l, 2) + pow(wrist_z, 2) - pow(l1r, 2) - pow(l2r, 2)) / (2 * l1r * l2r));
    double j3b = -j3a; // expands to -acos((l^2+wrist_z^2-L1r^2-L2r^2)/(2*L1r*L2r)) as per keenan's notes
    double j3ao = j3a + M_PI / 2;
    double j3bo = j3b + M_PI / 2;

    double k1a = l1r + l2r * cos(j3a);
    double k2a = l2r * sin(j3a);
    double k1b = l1r + l2r * cos(j3b);
    double k2b = l2r * sin(j3b);

    double j2a = atan2(wrist_z, l) - atan2(k2a, k1a);
    double j2b = atan2(wrist_z, l) - atan2(k2b, k1b);
    double j2ao = j2a - M_PI / 2;
    double j2bo = j2b - M_PI / 2;

    Eigen::Matrix4d t01 = sub_dh(0, 0, 0, j1);
    Eigen::Matrix4d t12 = sub_dh(M_PI / 2, 0, 0, j2bo + M_PI / 2);
    Eigen::Matrix4d t23 = sub_dh(0, l1r, 0, j3bo - M_PI / 2);
    Eigen::Matrix4d t02 = t01 * t12;
    Eigen::Matrix4d t03_wrist = t02 * t23;
    Eigen::Matrix3d r03_wrist = t03_wrist.topLeftCorner<3, 3>(); // R03_wrist = T03_wrist(1:3,1:3); in matlab
    Eigen::Matrix3d r07r = t07r.topLeftCorner<3, 3>(); // see above
    Eigen::Matrix3d r37r = r03_wrist.inverse() * r07r;

    double j4 = atan2(r37r(1, 2), r37r(0, 2));
    double j5 = atan2(-r37r(2, 2), r37r(0, 2) / cos(j4));
    double j6 = atan2(-r37r(2, 1) / cos(j5), r37r(2, 0) / cos(j5));

    std::array<double, 6> new_joints = { -j1, -j2bo, -j3bo, -j4, -j5, -j6 };
    return new_joints;
  }
} // namespace nova_ik_controller

#include "class_loader/register_macro.hpp"

CLASS_LOADER_REGISTER_CLASS(
    nova_ik_controller::NovaIKController, controller_interface::ControllerInterface)
