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
  /// The inverse of the global orientation the kinematics origin frame wants to rotate to for a roll,pitch,yaw of 0,0,0
  const auto ENDEFFECTOR_BASIS_INVERSE = tf2::Matrix3x3(tf2::Quaternion(-0.5, 0.5, -0.5, -0.5)).inverse();

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

namespace nova_ik_controller
{
  using namespace std::chrono_literals;
  using controller_interface::interface_configuration_type;
  using controller_interface::InterfaceConfiguration;
  using lifecycle_msgs::msg::State;

  NovaIKController::NovaIKController() : controller_interface::ChainableControllerInterface() {}

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

  std::vector<hardware_interface::CommandInterface> NovaIKController::on_export_reference_interfaces() {

    std::vector<hardware_interface::CommandInterface> reference_interfaces;
    reference_interfaces_.reserve(POSE_COMPONENTS.size());

    for (unsigned int i = 0; i < POSE_COMPONENTS.size(); i++) {
      const auto name = POSE_COMPONENTS[i];
      reference_interfaces.push_back(hardware_interface::CommandInterface(get_node()->get_name(), name,
                                                                          &reference_interfaces_[i]));
    }

    return reference_interfaces;
  }

  controller_interface::return_type NovaIKController::update_reference_from_subscribers(
    const rclcpp::Time &time, const rclcpp::Duration &period) {
    return controller_interface::return_type::OK;
  }

  controller_interface::return_type NovaIKController::update_and_write_commands(
      const rclcpp::Time &time, const rclcpp::Duration &period)
  {
    auto logger = get_node()->get_logger();
    if (get_lifecycle_state().id() == State::PRIMARY_STATE_INACTIVE)
    {
      return controller_interface::return_type::OK;
    }

    // Target pose from reference interfaces
    tf2::Vector3 reference_origin = tf2::Vector3(
      reference_interfaces_[0],
      reference_interfaces_[1],
      reference_interfaces_[2]);
    tf2::Quaternion reference_quat = tf2::Quaternion(
      reference_interfaces_[3],
      reference_interfaces_[4],
      reference_interfaces_[5],
      reference_interfaces_[6]);
    tf2::Transform reference_target_pose = tf2::Transform(reference_quat, reference_origin);

    // TODO: Retrieve these values from the robot description, rather than specifying them directly
    // TODO: Or even simpler, just put these in params!!!
    constexpr std::array<double, 3> lengths = {0.5, 0.68332, 0.11073};
    auto joint_angles = calculate_ik(reference_target_pose, lengths);

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

    RCLCPP_INFO(logger, "Created twist stamped subscription");

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

    return controller_interface::CallbackReturn::SUCCESS;
  }

  controller_interface::CallbackReturn NovaIKController::on_deactivate(
      const rclcpp_lifecycle::State &)
  {
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
    return true;
  }

  controller_interface::CallbackReturn NovaIKController::on_shutdown(
      const rclcpp_lifecycle::State &)
  {
    return controller_interface::CallbackReturn::SUCCESS;
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
    nova_ik_controller::NovaIKController, controller_interface::ChainableControllerInterface)
