#ifndef NOVA_END_EFFECTOR_CONTROLLER__NOVA_END_EFFECTOR_CONTROLLER_HPP_
#define NOVA_END_EFFECTOR_CONTROLLER__NOVA_END_EFFECTOR_CONTROLLER_HPP_

#include "controller_interface/controller_interface.hpp"
#include "controller_interface/chainable_controller_interface.hpp"
#include "visibility_control.h"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/state.hpp"
#include "realtime_tools/realtime_buffer.hpp"

#include <nova_end_effector_controller/nova_end_effector_controller_parameters.hpp>

namespace nova_end_effector_controller {
    class NovaEndEffectorController : public controller_interface::ChainableControllerInterface {
    public:
        NovaEndEffectorController();
        
        controller_interface::InterfaceConfiguration command_interface_configuration() const override;

        controller_interface::InterfaceConfiguration state_interface_configuration() const override;

        controller_interface::CallbackReturn on_init() override;

        controller_interface::CallbackReturn on_configure(
            const rclcpp_lifecycle::State & previous_state) override;

        controller_interface::CallbackReturn on_activate(
            const rclcpp_lifecycle::State & previous_state) override;

        controller_interface::CallbackReturn on_deactivate(
            const rclcpp_lifecycle::State & previous_state) override;

        controller_interface::CallbackReturn on_cleanup(
            const rclcpp_lifecycle::State & previous_state) override;

        controller_interface::CallbackReturn on_error(
            const rclcpp_lifecycle::State & previous_state) override;

        controller_interface::CallbackReturn on_shutdown(
            const rclcpp_lifecycle::State & previous_state) override;
    protected:
        struct JointHandle
        {
            std::string name;
            std::reference_wrapper<const hardware_interface::LoanedStateInterface> state_pos;
            std::reference_wrapper<const hardware_interface::LoanedStateInterface> state_vel;
            std::reference_wrapper<hardware_interface::LoanedCommandInterface> command;
        };

        const char *joint_command_type() const;

        controller_interface::CallbackReturn configure_joints(
            const std::vector<std::string> &joint_names,
            std::vector<JointHandle> &registered_handles);

        bool subscriber_is_active_ = false; // not sure what this is for yet
        bool is_halted = false;

    private:
        // Joints being used by the controller. Order should match that of the joint name definitions in parameters
        std::vector<JointHandle> registered_joint_handles_;
        
        // Parameters from ROS for nova_end_effector_controller
        std::shared_ptr<ParamListener> param_listener_{};
        Params params_{}; // TODO: Find the include for this
    };
};
#endif