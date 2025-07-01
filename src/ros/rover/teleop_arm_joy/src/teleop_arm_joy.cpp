/**
 * @file teleop_arm_joy.cpp
 * @brief Teleop Arm Joy node to translate Joy messages from /joy to commands for the arm.
 * Edited by Abby
 */

#include "teleop_arm_joy/teleop_arm_joy.hpp"

#include "colors.h"
#include "teleop_arm_joy/control_modes/ControlModeManager.hpp"

using namespace std::chrono_literals;

namespace
{
  // constexpr auto BUTTON_DEBOUNCE_INTERVAL = std::chrono::milliseconds(50);
  // constexpr auto DEFAULT_FK_VELOCITY_TOPIC = "/arm_fk_velocity_target";
  // constexpr auto DEFAULT_IK_TWIST_TOPIC = "/arm_ik_twist_stamped";
  // constexpr auto DEFAULT_AUTO_TYPING_TOPIC = "/test";
}

using std::placeholders::_1;
using std::placeholders::_2;

namespace teleop_arm_joy
{

TeleopArmJoy::TeleopArmJoy(const rclcpp::NodeOptions &options)
    : Node("teleop_arm_joy_node", options)
{
}

void TeleopArmJoy::initialize(const std::weak_ptr<rclcpp::Executor>& executor, const std::shared_ptr<TeleopArmJoy>& self) {
  // Create publishers
  param_listener_ = std::make_shared<ParamListener>(shared_from_this());
  params_ = param_listener_->get_params();

  inputs_ = InputManager();
//  inputs_.get_buttons().add("locked", std::static_pointer_cast<Input<bool>>(locked_));

  input_source_manager_ = std::make_shared<InputSourceManager>(shared_from_this(), executor);
  input_source_manager_->configure(param_listener_, inputs_);

  control_mode_manager_ = std::make_shared<ControlModeManager>(shared_from_this(), executor);
  control_mode_manager_->configure(inputs_);

  commands_ = std::make_shared<CommandManager>(shared_from_this(), self);
  commands_->configure(inputs_);
}

void TeleopArmJoy::log_all_inputs() {
  for (const auto& axis : inputs_.get_axes()) {
    if (!axis)
      continue;

    if (axis->changed()) {
      RCLCPP_INFO(get_logger(), C_INPUT "  %s\t%f", axis->get_name().c_str(), axis->value());
    }
  }
  for (const auto& button : inputs_.get_buttons()) {
    if (!button)
      continue;

    if (button->changed()) {
      RCLCPP_INFO(get_logger(), C_INPUT "  %s\t%d", button->get_name().c_str(), button->value());
    }
  }
  for (auto& [name, event] : inputs_.get_events()) {
    if (!event)
      continue;

    if (event->is_invoked()) {
      RCLCPP_INFO(get_logger(), C_QUIET "  %s invoked!", name.c_str());
    }
  }
}

[[noreturn]] void TeleopArmJoy::service_input_updates() {
  rclcpp::Time previous = this->now();

  // TODO: killing the thread
  while (true) {
    const auto now = input_source_manager_->wait_for_update();
    const auto period = now - previous;

    // TODO: Update inputs here
    input_source_manager_->update(now);
    inputs_.update(now);
    update_state();


    // Log inputs
    if (params_.log_inputs) {
      log_all_inputs();
    }

    control_mode_manager_->update(now, period);

    // TODO: enforce max update rate here
    previous = now;
  }
}

std::shared_ptr<const rclcpp::Node> TeleopArmJoy::get_node() const {
  return shared_from_this();
}

const InputManager& TeleopArmJoy::get_inputs() const {
  return inputs_;
}

const std::shared_ptr<ControlModeManager> TeleopArmJoy::get_control_modes() const {
  return control_mode_manager_;
}

void TeleopArmJoy::update_state() {
  auto logger = get_logger();

//  if (!locked_->value()) {
//    if (inputs_.get_buttons()["lock"]->value()) {
//      locked_->set(true);
//      // RCLCPP_INFO(logger, "Locked");
//    }
//  }
//  else {
//    if (inputs_.get_buttons()["unlock"]->value()) {
//      locked_->set(false);
//      // RCLCPP_INFO(logger, "Unlocked");
//    }
//  }
}

TeleopArmJoy::~TeleopArmJoy() {
  control_mode_manager_.reset();
}

}

int main(int argc, char *argv[]) {
  rclcpp::init(argc, argv);
  const auto node = std::make_shared<teleop_arm_joy::TeleopArmJoy>();
  const auto executor = std::make_shared<rclcpp::executors::MultiThreadedExecutor>();
  executor->add_node(node);

  std::cout << "\n";

  node->initialize(executor, node);

  {
    std::thread main_update_thread(&teleop_arm_joy::TeleopArmJoy::service_input_updates, node);
    // main_update_thread.detach();

    // rclcpp::spin(node);
    executor->spin();
  }

  rclcpp::shutdown();
  return 0;
}
