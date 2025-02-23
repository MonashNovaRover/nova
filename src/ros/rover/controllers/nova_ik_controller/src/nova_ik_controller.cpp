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
//#include "tf2_eigen/tf2_eigen/tf2_eigen.hpp"

namespace
{
} // namespace

using std::placeholders::_1;

namespace nova_ik_controller
{
  using namespace std::chrono_literals;
  using controller_interface::interface_configuration_type;
  using controller_interface::InterfaceConfiguration;
  using lifecycle_msgs::msg::State;

  NovaIKController::NovaIKController() : controller_interface::ControllerInterface(), node("ik") {}

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

    // TODO: Add state interfaces for setting the initial twistmapper position

    return {interface_configuration_type::INDIVIDUAL, conf_names};
  }

  controller_interface::return_type NovaIKController::update(
      const rclcpp::Time &time, const rclcpp::Duration &period)
  {
    auto logger = node.get_logger();
    if (get_lifecycle_state().id() == State::PRIMARY_STATE_INACTIVE)
    {
      if (!is_halted)
      {
        halt();
        is_halted = true;
      }

      return controller_interface::return_type::OK;
    }

    for (const auto &joint_handle : registered_joint_handles_)
    {
      float joint_speed = 0.0;
      RCLCPP_INFO(logger, "%s speed: %f", joint_handle.name.c_str(), joint_speed);
      joint_handle.command.get().set_value(joint_speed);
    }

    // feed inputs into IK

    return controller_interface::return_type::OK;
  }

  controller_interface::CallbackReturn NovaIKController::on_configure(
      const rclcpp_lifecycle::State &)
  {
    auto logger = node.get_logger();

    // update parameters if they have changed
    if (param_listener_->is_old(params_))
    {
      params_ = param_listener_->get_params();
      RCLCPP_INFO(logger, "Parameters were updated");
    }
    if (params_.joint_names.empty())
    {
      RCLCPP_ERROR(logger, "Joint names parameter is empty!");
      return controller_interface::CallbackReturn::ERROR;
    }

    // TODO: maybe limit position so arm doesn't collide?
    if (!reset())
    {
      return controller_interface::CallbackReturn::ERROR;
    }

    // TODO: insert correct subscriber here
    teleop_sub = node.create_subscription<tf2_msgs::msg::TFMessage>("aaaa", rclcpp::SystemDefaultsQoS(), std::bind(&NovaIKController::teleop_callback, this, _1));

    previous_update_timestamp_ = node.get_clock()->now();
    return controller_interface::CallbackReturn::SUCCESS;
  }

  controller_interface::CallbackReturn NovaIKController::on_activate(
      const rclcpp_lifecycle::State &)
  {
    const auto joints_result = configure_joints(params_.joint_names, registered_joint_handles_);

    if (joints_result == controller_interface::CallbackReturn::ERROR)
    {
      RCLCPP_ERROR(
          node.get_logger(),
          "Some joint interfaces are non existent");
      return controller_interface::CallbackReturn::ERROR;
    }
    is_halted = false;
    subscriber_is_active_ = true;

    // TODO: setup sub and pub
    RCLCPP_DEBUG(node.get_logger(), "Subscriber and publisher are now active.");
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
    // ! Don't do this!!!! We will send the arm flying to 0,0,0,0,0,0
    // for (const auto &joint_handle : registered_joint_handles_)
    // {
    //   joint_handle.command.get().set_value(0.0);
    // }
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
    auto logger = node.get_logger();

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
        [&joint_name, &command_interface_name](const auto &interface)
        {
          return interface.get_prefix_name() == joint_name &&
                 interface.get_interface_name() == command_interface_name;
        });

      if (command_handle == command_interfaces_.end())
      {
        RCLCPP_ERROR(logger, "Unable to obtain joint command handle for %s", joint_name.c_str());
        return controller_interface::CallbackReturn::ERROR;
      }

      registered_handles.emplace_back(
        JointHandle{joint_name, std::ref(*command_handle)});
    }

    return controller_interface::CallbackReturn::SUCCESS;
  }

  // See Keenan's IK notes
  // TODO: remember to add something for the effector pose
  void NovaIKController::calculate_ik(tf2_msgs::msg::TFMessage frame, geometry_msgs::msg::Pose pose, const double lengths[3], double joints[6])
  {
    double x = pose.position.x;
    double y = pose.position.y;
    double z = pose.position.z;
    double roll, pitch, yaw;

    tf2::Quaternion tf2_quat;
    tf2::fromMsg(pose.orientation, tf2_quat);
    tf2::Matrix3x3(tf2_quat).getRPY(roll, pitch, yaw);
    pitch += 90;

    double l1r = lengths[0];
    double l2r = lengths[1];
    double l3 = lengths[2];

    Eigen::Matrix3d rz_alp { {cosd(yaw), -sind(yaw), 0}, {sind(yaw), cosd(yaw), 0}, {0, 0, 1} }; // Yaw
    Eigen::Matrix3d ry_beta { {cosd(pitch), 0, sind(pitch)}, {0, 1, 0}, { -sind(pitch), 0, cosd(pitch)} }; // Pitch
    Eigen::Matrix3d rx_gam { {1, 0, 0}, {0, cosd(roll), -sind(roll)}, {0, sind(roll), cosd(roll)} }; // Roll
    Eigen::Matrix3d rxyz = rz_alp * ry_beta * rx_gam; // rxyz orientation matrix

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

    double new_joints[6] = { j1, j2bo, j3bo, j4, j5, j6 };
    // converts all elements in new_joints from radians to degrees, then puts them into joints to be returned
    for (int i = 0; i < 6; i++)
      joints[i] = new_joints[i] / (M_PI / 180);
    }

    void NovaIKController::teleop_callback(tf2_msgs::msg::TFMessage msg)
    {
      auto logger = node.get_logger();
    RCLCPP_DEBUG(logger, "Message received");

    // decompose the message

    // send it to IK

    // forward the output of IK into the FK controller
  }
} // namespace nova_ik_controller

#include "class_loader/register_macro.hpp"

CLASS_LOADER_REGISTER_CLASS(
    nova_ik_controller::NovaIKController, controller_interface::ControllerInterface)
