#include "nova_end_effector_controller/nova_end_effector_controller.hpp"
#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "lifecycle_msgs/msg/state.hpp"
#include "rclcpp/logging.hpp"
#include "joint_limits/joint_limits_rosparam.hpp"

namespace nova_end_effector_controller {
    using controller_interface::interface_configuration_type;
    using controller_interface::InterfaceConfiguration;
    using hardware_interface::HW_IF_POSITION;
    using hardware_interface::HW_IF_VELOCITY;
    using lifecycle_msgs::msg::State;

    const char *NovaEndEffectorController::joint_command_type() const {
        return params_.use_position_control ? HW_IF_POSITION : HW_IF_VELOCITY;
    }

    NovaEndEffectorController::NovaEndEffectorController() {}

    controller_interface::CallbackReturn NovaEndEffectorController::on_init()  {
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
        
        return controller_interface::CallbackReturn::SUCCESS;
    }

    controller_interface::CallbackReturn NovaEndEffectorController::on_configure(
        const rclcpp_lifecycle::State & previous_state
    )  {
        // TODO: Read parameters
        return controller_interface::CallbackReturn::SUCCESS;
    }

    controller_interface::InterfaceConfiguration NovaEndEffectorController::command_interface_configuration() const  {
        std::vector<std::string> conf_names;
        for (const auto &joint_name : params_.joint_names)
        {
            conf_names.push_back(joint_name + "/" + joint_command_type());
        }
        return {interface_configuration_type::INDIVIDUAL, conf_names};
    }

    controller_interface::InterfaceConfiguration NovaEndEffectorController::state_interface_configuration() const  {
        std::vector<std::string> conf_names;
        for (const auto &joint_name: params_.joint_names) {
            conf_names.push_back(joint_name + "/" + HW_IF_POSITION);
            conf_names.push_back(joint_name + "/" + HW_IF_VELOCITY);
        }
        return {interface_configuration_type::INDIVIDUAL, conf_names};
    }

    controller_interface::CallbackReturn NovaEndEffectorController::on_activate(
        const rclcpp_lifecycle::State & previous_state
    )  {
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

    controller_interface::CallbackReturn NovaEndEffectorController::on_deactivate(
        const rclcpp_lifecycle::State & previous_state
    )  {
        return controller_interface::CallbackReturn::SUCCESS;
    }

    controller_interface::CallbackReturn NovaEndEffectorController::on_cleanup(
        const rclcpp_lifecycle::State & previous_state
    )  {
        return controller_interface::CallbackReturn::SUCCESS;
    }

    controller_interface::CallbackReturn NovaEndEffectorController::on_error(
        const rclcpp_lifecycle::State & previous_state
    )  {
        return controller_interface::CallbackReturn::SUCCESS;
    }

    controller_interface::CallbackReturn NovaEndEffectorController::on_shutdown(
        const rclcpp_lifecycle::State & previous_state
    )  {
        return controller_interface::CallbackReturn::SUCCESS;
    }

    controller_interface::CallbackReturn NovaEndEffectorController::configure_joints(
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
}

