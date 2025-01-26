#include <memory>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "nova_arm_controller/nova_arm_controller.hpp"
#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "lifecycle_msgs/msg/state.hpp"
#include "rclcpp/logging.hpp"
#include "tf2/LinearMath/Quaternion.h"

namespace
{
  // communication with teleop-arm?
  /*constexpr auto DEFAULT_INPUT_TOPIC_TWIST = "/cmd_vel";
  constexpr auto DEFAULT_INPUT_TOPIC = "/drive_input";
  constexpr auto DEFAULT_COMMAND_OUT_TOPIC = "~/cmd_vel_out";
  constexpr auto DEFAULT_ODOMETRY_TOPIC = "~/odom";
  constexpr auto DEFAULT_TRANSFORM_TOPIC = "/tf";
  */
} // namespace


// need to drive J4, J5, J6 based on mesages from teleop-arm
// start with just wrist roll? (I think that's J6?)

namespace nova_arm_controller
{
  using namespace std::chrono_literals;
  using controller_interface::interface_configuration_type;
  using controller_interface::InterfaceConfiguration;
  using hardware_interface::HW_IF_POSITION;
  using hardware_interface::HW_IF_VELOCITY;
  using hardware_interface::HW_IF_EFFORT;
  using lifecycle_msgs::msg::State;

  NovaArmController::NovaArmController() : controller_interface::ControllerInterface() {}

  const char *NovaArmController::joint_feedback_type() const
  {
    return params_.joint_position_feedback ? HW_IF_POSITION : HW_IF_VELOCITY;
  }

  const char *NovaArmController::linear_feedback_type() const
  {
    return params_.linear_position_feedback ? HW_IF_POSITION : HW_IF_VELOCITY;
  }

  controller_interface::CallbackReturn NovaArmController::on_init()
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

  InterfaceConfiguration NovaArmController::command_interface_configuration() const
  {
    std::vector<std::string> conf_names;
    for (const auto &joint_name : params_.joint_names)
    {
      conf_names.push_back(joint_name + "/" + HW_IF_VELOCITY);
    }
    for (const auto &joint_name : params_.joint_names)
    {
      conf_names.push_back(joint_name + "/" + HW_IF_EFFORT);
    }
    return {interface_configuration_type::INDIVIDUAL, conf_names};
  }

  InterfaceConfiguration NovaArmController::state_interface_configuration() const
  {
    std::vector<std::string> conf_names;
    for (const auto &joint_name : params_.joint_names)
    {
      conf_names.push_back(joint_name + "/" + joint_feedback_type());
    }
    return {interface_configuration_type::INDIVIDUAL, conf_names};
  }

  controller_interface::return_type NovaArmController::update(
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

    //RCLCPP_INFO(logger, "Left wheel speed: %f, Right wheel speed: %f", left_speed / params_.wheel_radius, right_speed / params_.wheel_radius);

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

    /*limiter_angular_ = SpeedLimiter(
        params_.angular.z.has_velocity_limits, params_.angular.z.has_acceleration_limits,
        params_.angular.z.has_jerk_limits, params_.angular.z.min_velocity,
        params_.angular.z.max_velocity, params_.angular.z.min_acceleration,
        params_.angular.z.max_acceleration, params_.angular.z.min_jerk, params_.angular.z.max_jerk);
        */ // need to do this for each joint? ^
    // TODO: maybe limit position so arm doesn't collide?
    if (!reset())
    {
      return controller_interface::CallbackReturn::ERROR;
    }

    // TODO: setup publishers?
    previous_update_timestamp_ = get_node()->get_clock()->now();
    return controller_interface::CallbackReturn::SUCCESS;
  }

  controller_interface::CallbackReturn NovaArmController::on_activate(
      const rclcpp_lifecycle::State &)
  {
    const auto joints_result =
        configure_joints(params_.joint_names, registered_joint_handles_, joint_feedback_type());
    // same for linear actuator?

    if (joints_result == controller_interface::CallbackReturn::ERROR)
    {
      RCLCPP_ERROR(
          get_node()->get_logger(),
          "Some joint interfaces are non existent");
      return controller_interface::CallbackReturn::ERROR;
    }


    is_halted = false;
    subscriber_is_active_ = true;

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
    // TODO: same for linear?
    return controller_interface::CallbackReturn::SUCCESS;
  }

  controller_interface::CallbackReturn NovaArmController::on_cleanup(
      const rclcpp_lifecycle::State &)
  {
    if (!reset())
    {
      return controller_interface::CallbackReturn::ERROR;
    }
    // ???

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
    // release the old queue
    subscriber_is_active_ = false;

    is_halted = false;
    return true;
  }

  controller_interface::CallbackReturn NovaArmController::on_shutdown(
      const rclcpp_lifecycle::State &)
  {
    //???
    return controller_interface::CallbackReturn::SUCCESS;
  }

  void NovaArmController::halt()
  {
    const auto halt_blcmds = [](auto &joint_handles)
    {
      for (const auto &joint_handle : joint_handles)
      {
        joint_handle.command.get().set_value(0.0); // XXX: check this
      }
    };

    halt_blcmds(registered_joint_handles_);
    // same for linear?
  }

  controller_interface::CallbackReturn NovaArmController::configure_joints(
      const std::vector<std::string> &joint_names,
      std::vector<JointHandle> &registered_handles, const char *feedback_type)
  {
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
      const auto interface_name = feedback_type;
      const auto state_handle = std::find_if(
          state_interfaces_.cbegin(), state_interfaces_.cend(),
          [&joint_name, &interface_name](const auto &interface)
          {
            return interface.get_prefix_name() == joint_name &&
                   interface.get_interface_name() == interface_name;
          });

      if (state_handle == state_interfaces_.cend())
      {
        RCLCPP_ERROR(logger, "Unable to obtain joint state handle for %s", joint_name.c_str());
        return controller_interface::CallbackReturn::ERROR;
      }

      const auto command_handle = std::find_if(
          command_interfaces_.begin(), command_interfaces_.end(),
          [&joint_name, &interface_name](const auto &interface)
          {
            return interface.get_prefix_name() == joint_name &&
                   interface.get_interface_name() == interface_name;
          });

      if (command_handle == command_interfaces_.end())
      {
        RCLCPP_ERROR(logger, "Unable to obtain joint command handle for %s", joint_name.c_str());
        return controller_interface::CallbackReturn::ERROR;
      }

      registered_handles.emplace_back(
          JointHandle{std::ref(*state_handle), std::ref(*command_handle)});
    }

    return controller_interface::CallbackReturn::SUCCESS;
  }
} // namespace nova_arm_controller

#include "class_loader/register_macro.hpp"

CLASS_LOADER_REGISTER_CLASS(
    nova_arm_controller::NovaArmController, controller_interface::ControllerInterface)
