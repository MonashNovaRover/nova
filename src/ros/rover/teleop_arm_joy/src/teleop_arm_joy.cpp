/**
 * @file teleop_arm_joy.cpp
 * @brief Teleop Arm Joy node to translate Joy messages from /joy to commands for the arm.
 * Edited by Abby
 */

#include "teleop_arm_joy/teleop_arm_joy.hpp"

#include "teleop_arm_joy/control_modes/ControlModeManager.hpp"

using namespace std::chrono_literals;

namespace
{
  constexpr auto BUTTON_DEBOUNCE_INTERVAL = std::chrono::milliseconds(50);
  constexpr auto DEFAULT_FK_VELOCITY_TOPIC = "/arm_fk_velocity_target";
  constexpr auto DEFAULT_IK_TWIST_TOPIC = "/arm_ik_twist_stamped";
  constexpr auto DEFAULT_AUTO_TYPING_TOPIC = "/test";
}

using std::placeholders::_1;
using std::placeholders::_2;

namespace teleop_arm_joy
{

TeleopArmJoy::TeleopArmJoy(const rclcpp::NodeOptions &options)
    : Node("teleop_arm_joy_node", options)
{
  // Create publishers

}

//
// void TeleopArmJoy::sendJointSpaceCommand()
// {
//   auto msg = std::make_unique<nova_interfaces::msg::ArmFkVelocityTargets>();
//
//   msg->header.stamp = this->now();
//
//   for (const auto& [joint_name, joint_config] : params_.joints.joint_definitions_map) {
//     msg->name.emplace_back(joint_name);
//
//     if (!axes.count(joint_name)) {
//       RCLCPP_WARN(this->get_logger(), "Axis for joint with name '%s' does not exist!", joint_name.c_str());
//       msg->velocity.emplace_back(0);
//       continue;
//     }
//
//     const float input = axes[joint_name]->value();
//     double velocity = static_cast<double>(input) * speed * joint_config.max_speed;
//
//     msg->velocity.emplace_back(velocity);
//   }
//
//   fk_velocity_pub->publish(std::move(msg));
//
//   auto ik_msg = std::make_unique<geometry_msgs::msg::TwistStamped>();
//
//   ik_msg->header.stamp = this->now();
//
//   auto linear = geometry_msgs::msg::Vector3();
//   linear.x = linear.y = linear.z = 0;
//
//   auto angular = geometry_msgs::msg::Vector3();
//   angular.x = angular.y = angular.z = 0;
//
//   ik_msg->twist.linear = linear;
//   ik_msg->twist.angular = angular;
//
//   ik_twist_pub->publish(std::move(ik_msg));
// }
//
// void TeleopArmJoy::sendTwistCommand() {
//   auto msg = std::make_unique<geometry_msgs::msg::TwistStamped>();
//
//   msg->header.stamp = this->now();
//
//   const auto linear_speed = speed * params_.control_modes.twist.linear_max;
//   auto linear = geometry_msgs::msg::Vector3();
//   linear.x = axes["twist_x"]->value() * linear_speed;
//   linear.y = axes["twist_y"]->value() * linear_speed;
//   linear.z = axes["twist_z"]->value() * linear_speed;
//
//   const auto angular_speed = speed * params_.control_modes.twist.angular_max;
//   auto angular = geometry_msgs::msg::Vector3();
//   angular.x = axes["twist_roll" ]->value() * angular_speed;
//   angular.y = axes["twist_pitch"]->value() * angular_speed;
//   angular.z = axes["twist_yaw"  ]->value() * angular_speed;
//
//   msg->twist.linear = linear;
//   msg->twist.angular = angular;
//
//   ik_twist_pub->publish(std::move(msg));
// }

// void TeleopArmJoy::sendHaltCommand()
// {
//   // Send all zeroes for joint space
//   auto msg = std::make_unique<nova_interfaces::msg::ArmFkVelocityTargets>();
//
//   msg->header.stamp = this->now();
//
//   for (const auto& [joint_name, joint_config] : params_.joints.joint_definitions_map) {
//     msg->name.emplace_back(joint_name);
//     msg->velocity.emplace_back(0.0);
//   }
//
//   fk_velocity_pub->publish(std::move(msg));
//
//   // Send halt command for IK
//   auto ik_msg = std::make_unique<geometry_msgs::msg::TwistStamped>();
//
//   ik_msg->header.stamp = this->now();
//
//   auto linear = geometry_msgs::msg::Vector3();
//   linear.x = linear.y = linear.z = 0;
//
//   auto angular = geometry_msgs::msg::Vector3();
//   angular.x = angular.y = angular.z = 0;
//
//   ik_msg->twist.linear = linear;
//   ik_msg->twist.angular = angular;
//
//   ik_twist_pub->publish(std::move(ik_msg));
//
//   //TODO: halt auto typing command
// }

void TeleopArmJoy::initialize(const std::weak_ptr<rclcpp::Executor>& executor) {
  inputs_ = InputManager();

  input_source_manager_ = std::make_shared<InputSourceManager>(shared_from_this(), executor);
  input_source_manager_->configure(inputs_);

  control_mode_manager_ = std::make_shared<ControlModeManager>(shared_from_this(), executor);
  control_mode_manager_->configure(inputs_);
}

[[noreturn]] void TeleopArmJoy::service_input_updates() {
  // TODO: killing the thread
  while (true) {
    input_source_manager_->wait_for_update();

    // TODO: Update here

    // TODO: enforce max update rate here
  }
}

TeleopArmJoy::~TeleopArmJoy() {
  control_mode_manager_.reset();
}

}

int main(int argc, char *argv[]) {
  rclcpp::init(argc, argv);
  const auto node = std::make_shared<teleop_arm_joy::TeleopArmJoy>();
  // node->initializeParams();

  const auto executor = std::make_shared<rclcpp::executors::MultiThreadedExecutor>();
  executor->add_node(node);

  node->initialize(executor);

  {
    std::thread main_update_thread(&teleop_arm_joy::TeleopArmJoy::service_input_updates, node);
    // main_update_thread.detach();

    // rclcpp::spin(node);
    executor->spin();
  }

  rclcpp::shutdown();
  return 0;
}
