#include <memory>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "nova_ik_controller/nova_ik_controller.hpp"
#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "lifecycle_msgs/msg/state.hpp"
#include "rclcpp/logging.hpp"

namespace
{
  constexpr auto DEFAULT_INPUT_TOPIC_IK_JOINT_VELOCITY = "/IK_TESTING"; // TODO: changeme
} // namespace

namespace nova_ik_controller
{
  using namespace std::chrono_literals;
  using controller_interface::interface_configuration_type;
  using controller_interface::InterfaceConfiguration;
  using hardware_interface::HW_IF_POSITION;
  using hardware_interface::HW_IF_VELOCITY;
  using hardware_interface::HW_IF_EFFORT;
  using lifecycle_msgs::msg::State;

  NovaIKController::NovaIKController() : controller_interface::ControllerInterface() {}

  const char *NovaIKController::joint_feedback_type() const
  {
    return params_.joint_position_feedback ? HW_IF_POSITION : HW_IF_VELOCITY;
  }

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
      conf_names.push_back(joint_name + "/" + HW_IF_VELOCITY);
    }
    for (const auto &joint_name : params_.joint_names)
    {
      conf_names.push_back(joint_name + "/" + HW_IF_EFFORT);
    }
    return {interface_configuration_type::INDIVIDUAL, conf_names};
  }

  InterfaceConfiguration NovaIKController::state_interface_configuration() const
  {
    std::vector<std::string> conf_names;
    for (const auto &joint_name : params_.joint_names)
    {
      conf_names.push_back(joint_name + "/" + joint_feedback_type());
    }
    return {interface_configuration_type::INDIVIDUAL, conf_names};
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

    for (const auto &joint_handle : registered_joint_handles_)
    {
      float joint_speed = 0.0;
      RCLCPP_INFO(logger, "%s speed: %f", joint_handle.name.c_str(), joint_speed);
      joint_handle.command.get().set_value(joint_speed);
    }

    return controller_interface::return_type::OK;
  }

  controller_interface::CallbackReturn NovaIKController::on_configure(
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

    // TODO: validate angular limits

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

  controller_interface::CallbackReturn NovaIKController::on_activate(
      const rclcpp_lifecycle::State &)
  {
    const auto joints_result =
        configure_joints(params_.joint_names, registered_joint_handles_, joint_feedback_type());

    if (joints_result == controller_interface::CallbackReturn::ERROR)
    {
      RCLCPP_ERROR(
          get_node()->get_logger(),
          "Some joint interfaces are non existent");
      return controller_interface::CallbackReturn::ERROR;
    }

    is_halted = false;
    subscriber_is_active_ = true;

    // TODO: setup sub and pub
    //RCLCPP_DEBUG(get_node()->get_logger(), "Subscriber and publisher are now active.");
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
    for (const auto &joint_handle : registered_joint_handles_)
    {
      joint_handle.command.get().set_value(0.0);
    }
  }

  controller_interface::CallbackReturn NovaIKController::configure_joints(
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
    //TODO: pos/vel/etc limits
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
                   interface.get_interface_name() == interface_name; //TODO: might need this to not be same as state interface
          });

      if (command_handle == command_interfaces_.end())
      {
        RCLCPP_ERROR(logger, "Unable to obtain joint command handle for %s", joint_name.c_str());
        return controller_interface::CallbackReturn::ERROR;
      }

      registered_handles.emplace_back(
          JointHandle{joint_name, std::ref(*state_handle), std::ref(*command_handle)});
    }

    return controller_interface::CallbackReturn::SUCCESS;
  }
} // namespace nova_ik_controller

#include "class_loader/register_macro.hpp"

CLASS_LOADER_REGISTER_CLASS(
    nova_ik_controller::NovaIKController, controller_interface::ControllerInterface)
