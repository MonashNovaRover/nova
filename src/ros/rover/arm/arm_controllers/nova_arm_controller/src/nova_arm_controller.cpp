#include <memory>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "nova_arm_controller/nova_arm_controller.hpp"
#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "lifecycle_msgs/msg/state.hpp"
#include "rclcpp/logging.hpp"
#include "joint_limits/joint_limits_rosparam.hpp"


namespace
{
  constexpr auto DEFAULT_INPUT_TOPIC_ARM_JOINT_VELOCITY = "/arm_fk_velocity_target";
} // namespace


namespace nova_arm_controller
{
using namespace std::chrono_literals;
using controller_interface::interface_configuration_type;
using controller_interface::InterfaceConfiguration;
using hardware_interface::HW_IF_POSITION;
using hardware_interface::HW_IF_VELOCITY;
using lifecycle_msgs::msg::State;

// NovaArmController::NovaArmController() : controller_interface::ChainableControllerInterface() {}

const char *NovaArmController::joint_command_type() const {
  return params_.use_position_control ? HW_IF_POSITION : HW_IF_VELOCITY;
}

controller_interface::CallbackReturn NovaArmController::on_init()
{
  RCLCPP_INFO(get_node()->get_logger(), "Controller Initialising");
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
  
  if (!this->joint_limiter.init(params_.joint_names, get_node())) {
    RCLCPP_ERROR(get_node()->get_logger(), "Failed to init joint limiter");
    return controller_interface::CallbackReturn::ERROR;
  }

  if (!this->collision_limiter.init(params_.joint_names, get_node(), get_robot_description())) {
    RCLCPP_ERROR(get_node()->get_logger(), "Failed to init collision limiter");
    return controller_interface::CallbackReturn::ERROR;
  }

  return controller_interface::CallbackReturn::SUCCESS;
}

InterfaceConfiguration NovaArmController::command_interface_configuration() const
{
  std::vector<std::string> conf_names;
  for (const auto &joint_name : params_.joint_names)
  {
    conf_names.push_back(joint_name + "/" + joint_command_type());
  }
  return {interface_configuration_type::INDIVIDUAL, conf_names};
}

InterfaceConfiguration NovaArmController::state_interface_configuration() const {
  std::vector<std::string> conf_names;
  for (const auto &joint_name: params_.joint_names) {
    conf_names.push_back(joint_name + "/" + HW_IF_POSITION);
    conf_names.push_back(joint_name + "/" + HW_IF_VELOCITY);
  }
  return {interface_configuration_type::INDIVIDUAL, conf_names};
}

std::vector<hardware_interface::CommandInterface> NovaArmController::on_export_reference_interfaces() {
  std::vector<hardware_interface::CommandInterface> reference_interfaces;
  RCLCPP_INFO(get_node()->get_logger(), "Export reference interfaces");

  const auto joint_count = params_.joint_names.size();
  reference_interfaces_.reserve(joint_count);

  // Either "position" or "velocity" based on params_.use_position_control
  const auto& interface_name_suffix = joint_command_type();

  // Make a velocity interface for each joint
  for (unsigned int i = 0; i < joint_count; i++) {
    const auto name = params_.joint_names[i] + "/" + interface_name_suffix;
    reference_interfaces.push_back(hardware_interface::CommandInterface(get_node()->get_name(), name,
                                                                        &reference_interfaces_[i]));
  }

  return reference_interfaces;
}

// Called before update_and_write_commands
controller_interface::return_type NovaArmController::update_reference_from_subscribers(const rclcpp::Time &time, const rclcpp::Duration &period) {
  // TODO: implement position control, and have it choose between position or velocity functions based on some state or parameter
  (void)time;(void)period; // slience unused vars
  return update_velocity_reference_from_subscribers();
}

controller_interface::return_type NovaArmController::update_velocity_reference_from_subscribers() {
  auto logger = get_node()->get_logger();

  RCLCPP_INFO_ONCE(get_node()->get_logger(), "Update velocity reference from subscribers");

  std::shared_ptr<nova_interfaces::msg::ArmFkVelocityTargets> last_msg;
  received_msg_ptr_.get(last_msg);

  if (last_msg == nullptr) {
    RCLCPP_WARN_ONCE(logger, "Velocity message received was a nullptr.");
    return controller_interface::return_type::OK;
  }

  if (last_msg->name.size() != last_msg->velocity.size()) {
    RCLCPP_WARN(logger, "Velocity message received had a different number of names and velocities.");
    return controller_interface::return_type::ERROR;
  }

  // Make map of joint name -> velocity (from message)
  auto velocities = std::map<std::string, double>();
  for (unsigned int i = 0; i < last_msg->name.size(); ++i) {
    velocities[last_msg->name[i]] = last_msg->velocity[i];
  }

  for (unsigned int i = 0; i < params_.joint_names.size(); i++)
  {
    const auto& joint_name = params_.joint_names[i];

    // Ensure the map contains the handle
    if (velocities.find(joint_name) == velocities.end()) {
      RCLCPP_WARN(logger, "Joint '%s' not defined in input message from teleop-arm-joy.", joint_name.c_str());
      reference_interfaces_[i] = 0;
      continue;
    }

    reference_interfaces_[i] = velocities[joint_name];
  }

  return controller_interface::return_type::OK;
}

// this assumes that the number of joints match
void NovaArmController::get_joint_states(trajectory_msgs::msg::JointTrajectoryPoint &current) {
  //TODO: maybe try to calculate accel as well - this is needed for jerk limits
  current.positions.resize(params_.joint_names.size());
  current.velocities.resize(params_.joint_names.size());

  for (unsigned int i = 0; i < registered_joint_handles_.size(); i++)
  {
    const auto& joint_handle = registered_joint_handles_[i];

    current.positions[i] = joint_handle.state_pos.get().get_value();
    current.velocities[i] = joint_handle.state_vel.get().get_value();
  }
  return;
}

controller_interface::return_type NovaArmController::update_and_write_commands(
    const rclcpp::Time &time, const rclcpp::Duration &period)
{
  auto logger = get_node()->get_logger();
  (void)time;(void)period; // slience unused vars

  // TODO: change implementation to use values from reference_interfaces_ rather than the subscriber message.
  // (anything related to the subscriber should not exist in this function)
  //RCLCPP_INFO(logger, "Update and write commands");

  if (get_lifecycle_state().id() == State::PRIMARY_STATE_INACTIVE)
  {
    if (!is_halted)
    {
      halt();
      is_halted = true;
    }

    return controller_interface::return_type::OK;
  }

  // TODO: Make sure we have state for halting and we halt when necessary

  if (registered_joint_handles_.size() != params_.joint_names.size()) {
    RCLCPP_ERROR(logger, "Assertion failed: %lu != %lu", registered_joint_handles_.size(), params_.joint_names.size());
    return controller_interface::return_type::ERROR;
  }

  trajectory_msgs::msg::JointTrajectoryPoint desired, current;

  if (this->joint_command_type() == HW_IF_POSITION) {
    desired.positions.resize(params_.joint_names.size());
  } else { // velocity
    desired.velocities.resize(params_.joint_names.size());
  }

  for (unsigned int i = 0; i < registered_joint_handles_.size(); i++)
  {
    const auto& joint_handle = registered_joint_handles_[i];

    // We use this assumption to index into the reference interface arrays using the same index
    if (joint_handle.name != params_.joint_names[i]) {
      RCLCPP_ERROR(logger, "Assertion failed: %s != %s", joint_handle.name.c_str(), params_.joint_names[i].c_str());
      return controller_interface::return_type::ERROR;
    }

    if (this->joint_command_type() == HW_IF_POSITION) {
      desired.positions[i] = std::isnan(reference_interfaces_[i]) ? joint_handle.state_pos.get().get_value()
        : reference_interfaces_[i];
    } else { // velocity
      desired.velocities[i] = std::isnan(reference_interfaces_[i]) ? 0.0
        : reference_interfaces_[i];
    }
  }
  this->get_joint_states(current);

  if (params_.use_limits) {
    this->joint_limiter.enforce(current, desired, period);
  }

  if (params_.use_collision_limits) {
    this->collision_limiter.enforce(current, desired, period);
  }

  for (unsigned int i = 0; i < registered_joint_handles_.size(); i++)
  {
    const auto& joint_handle = registered_joint_handles_[i];
    double reference_value;
    if (this->joint_command_type() == HW_IF_POSITION) {
      reference_value = desired.positions.at(i);
    } else { // velocity
      reference_value = desired.velocities.at(i);
    }
    if (std::isnan(reference_value)) {
      // When dealing with invalid or missing inputs, don't move
      const auto halt_value = params_.use_position_control ? joint_handle.state_pos.get().get_value() : 0.0;
      RCLCPP_WARN(get_node()->get_logger(), "Missing or NaN input received. Trying to do nothing with value %f for "
                                            "joint \"%s\".", halt_value, joint_handle.name.c_str());
      if (!joint_handle.command.get().set_value(halt_value)) {
        return controller_interface::return_type::ERROR;
      }

      continue;
    }

    if (!joint_handle.command.get().set_value(reference_value)) {
      return controller_interface::return_type::ERROR;
    }

    // RCLCPP_INFO(logger, "%s reference value: %f", joint_handle.name.c_str(), reference_value);
  }

  return controller_interface::return_type::OK;
}

controller_interface::CallbackReturn NovaArmController::on_configure(
    const rclcpp_lifecycle::State &)
{
  auto logger = get_node()->get_logger();

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

  if (!reset())
  {
    return controller_interface::CallbackReturn::ERROR;
  }

  trajectory_msgs::msg::JointTrajectoryPoint current;
  this->get_joint_states(current);
  
  if (!this->joint_limiter.configure(current)) {
    RCLCPP_ERROR(logger, "Failed to configure joint limiter!");
    return controller_interface::CallbackReturn::ERROR;
  }
  if (!this->collision_limiter.configure(current)) {
    RCLCPP_ERROR(logger, "Failed to configure collision limiter!");
    return controller_interface::CallbackReturn::ERROR;
  }

  // TODO: setup publishers?
  RCLCPP_INFO(get_node()->get_logger(), "Creating subscriber");

  input_subscriber_ = get_node()->create_subscription<nova_interfaces::msg::ArmFkVelocityTargets>(
  DEFAULT_INPUT_TOPIC_ARM_JOINT_VELOCITY, rclcpp::SystemDefaultsQoS(),
  [this](const std::shared_ptr<nova_interfaces::msg::ArmFkVelocityTargets> msg) -> void
  {
    if (!subscriber_is_active_)
    {
      RCLCPP_WARN_ONCE(
          get_node()->get_logger(), "Can't accept new commands. subscriber is inactive");
      return;
    }
    if ((msg->header.stamp.sec == 0) && (msg->header.stamp.nanosec == 0))
    {
      RCLCPP_WARN_ONCE(
          get_node()->get_logger(),
          "Received message with zero timestamp, setting it to current "
          "time, this message will only be shown once");
      msg->header.stamp = get_node()->get_clock()->now();
    }

    received_msg_ptr_.set(std::move(msg));
  });

  // Set number of reference interface.
  // https://github.com/ros-controls/ros2_control_demos/blob/332ede0ee44f9c3382666df91a0b7d49a368652f/example_12/controllers/src/passthrough_controller.cpp#L78
  const unsigned int reference_interface_count = params_.joint_names.size();
  command_interfaces_.reserve(reference_interface_count);
  reference_interfaces_.resize(reference_interface_count, std::numeric_limits<double>::quiet_NaN());

  previous_update_timestamp_ = get_node()->get_clock()->now();
  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn NovaArmController::on_activate(
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

  // Reset reference interfaces
  // https://github.com/ros-controls/ros2_control_demos/blob/f09c24040243973d48f7a102afc70559b2dc3908/example_12/controllers/src/passthrough_controller.cpp#L115
  std::fill(
    reference_interfaces_.begin(), reference_interfaces_.end(),
    std::numeric_limits<double>::quiet_NaN());


  if (params_.use_position_control) {
    // Set all joint command interfaces to be the current state interface values
    for (auto& joint : registered_joint_handles_) {
      if (!joint.command.get().set_value(joint.state_pos.get().get_value())) {
        return controller_interface::CallbackReturn::ERROR;
      }
    }
  }

  // TODO: setup sub and pub
  //RCLCPP_DEBUG(get_node()->get_logger(), "Subscriber and publisher are now active.");
  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn NovaArmController::on_deactivate(
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

bool NovaArmController::on_set_chained_mode(bool chained_mode) {
  // This method is called when the chained mode is set.
  (void)chained_mode; // silence unused vars
  return true;
}

controller_interface::CallbackReturn NovaArmController::on_cleanup(
    const rclcpp_lifecycle::State &)
{
  if (!reset())
  {
    return controller_interface::CallbackReturn::ERROR;
  }

  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn NovaArmController::on_error(const rclcpp_lifecycle::State &)
{
  if (!reset())
  {
    return controller_interface::CallbackReturn::ERROR;
  }
  return controller_interface::CallbackReturn::SUCCESS;
}

bool NovaArmController::reset()
{
  // TODO: Confirm behaviour in a failure state, and ensure that we stay failed for critical errors

  // release the old queue
  subscriber_is_active_ = false;

  is_halted = false;
  return true;
}

controller_interface::CallbackReturn NovaArmController::on_shutdown(
    const rclcpp_lifecycle::State &)
{
  return controller_interface::CallbackReturn::SUCCESS;
}

void NovaArmController::halt()
{
  for (const auto &joint_handle : registered_joint_handles_)
  {
    bool _ = joint_handle.command.get().set_value(0.0);
  }
}

controller_interface::CallbackReturn NovaArmController::configure_joints(
    const std::vector<std::string> &joint_names,
    std::vector<JointHandle> &registered_handles)
{
  RCLCPP_INFO(get_node()->get_logger(), "Configure joints");

  auto logger = get_node()->get_logger();

  if (joint_names.empty())
  {
    RCLCPP_ERROR(logger, "No joint names specified");
    return controller_interface::CallbackReturn::ERROR;
  }

  // register handles
  registered_handles.reserve(joint_names.size());
  for (const auto &joint_name : joint_names)
  {
    const auto command_interface_name = joint_command_type();

    //TODO: DRY (same code twice for pos and vel)
    const auto pos_state_handle = std::find_if(
        state_interfaces_.cbegin(), state_interfaces_.cend(),
        [&joint_name](const auto &interface)
        {
          return interface.get_prefix_name() == joint_name &&
                 interface.get_interface_name() == HW_IF_POSITION;
        });

    const auto vel_state_handle = std::find_if(
        state_interfaces_.cbegin(), state_interfaces_.cend(),
        [&joint_name](const auto &interface)
        {
          return interface.get_prefix_name() == joint_name &&
                 interface.get_interface_name() == HW_IF_VELOCITY;
        });

    if (pos_state_handle == state_interfaces_.cend())
    {
      RCLCPP_ERROR(logger, "Unable to obtain position joint state handle for %s", joint_name.c_str());
      return controller_interface::CallbackReturn::ERROR;
    }
    
    if (vel_state_handle == state_interfaces_.cend())
    {
      RCLCPP_ERROR(logger, "Unable to obtain velocity joint state handle for %s", joint_name.c_str());
      return controller_interface::CallbackReturn::ERROR;
    }

    // TODO: Change this filter to be useful, and not get the same as the state_interface
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
        JointHandle{
          joint_name,
          std::ref(*pos_state_handle),
          std::ref(*vel_state_handle),
          std::ref(*command_handle)
          }
        );
  }

  return controller_interface::CallbackReturn::SUCCESS;
}

} // namespace nova_arm_controller

#include "class_loader/register_macro.hpp"

CLASS_LOADER_REGISTER_CLASS(
    nova_arm_controller::NovaArmController, controller_interface::ChainableControllerInterface)
