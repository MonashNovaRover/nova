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
} // namespace

namespace nova_ik_controller
{
  using namespace std::chrono_literals;
  using controller_interface::interface_configuration_type;
  using controller_interface::InterfaceConfiguration;
  using lifecycle_msgs::msg::State;

  NovaIKController::NovaIKController() : controller_interface::ControllerInterface(), node("ik") {}

  const char *NovaIKController::joint_feedback_type() const
  {
  }

  controller_interface::CallbackReturn NovaIKController::on_init()
  {
    try
    {
      // Create the parameter listener and get the parameters
      param_listener_ = std::make_shared<ParamListener>(node);
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
  }

  InterfaceConfiguration NovaIKController::state_interface_configuration() const
  {
  
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
	teleop_sub = node.create_subscription<tf2_msgs::msg::TFMessage>("aaaa", rclcpp::SystemDefaultsQoS(), nullptr);

    previous_update_timestamp_ = node.get_clock()->now();
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
    for (const auto &joint_handle : registered_joint_handles_)
    {
      joint_handle.command.get().set_value(0.0);
    }
  }

  controller_interface::CallbackReturn NovaIKController::configure_joints(
      const std::vector<std::string> &joint_names,
      std::vector<JointHandle> &registered_handles, const char *feedback_type)
  {
    auto logger = node.get_logger();

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


  void NovaIKController::calculate_ik(tf2_msgs::msg::TFMessage frame, geometry_msgs::msg::Pose wristPose, geometry_msgs::msg::Pose effPose)
  {
	
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
