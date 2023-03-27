/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jess Hepworth, Jory Braun
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include class header
#include "arm_driver.h"

// Include other headers
#include "print/print.h"
#include "config/rosconfig.h"
#include "../arm_configuration.h"

// Use the standard namespaces
using std::placeholders::_1;


// Receives the desired commands for the CMDs and sends to CMDs
void ArmDriver::joint_velocities_callback (const sensor_msgs::msg::JointState::SharedPtr msg)
{
    for (uint16_t i = 0; i < arm_model->num_joints; i++) {
        arm_model->drivers[i]->drive(msg->velocity[i]);
    }
}
// Reset the internal velocities
void ArmDriver::joint_velocities_deadline_callback()
{
    RCLCPP_WARN(this->get_logger(), "control/joint_velocities subscription deadline missed");
    for (uint16_t i = 0; i < arm_model->num_joints; i++) {
        arm_model->drivers[i]->drive(0);
    }
}

// Receives the desired commands for the CMDs and sends to CMDs
void ArmDriver::endeffector_input_callback (const core::msg::EndEffectorInput::SharedPtr msg)
{
    // First 6 joints are handled by joint_velocities_callback

    // Receiving data for end effector
    // If the current arm configuration is cycloidal wrist + ER end effector, reduce the PWM by 50%
    // This accounts for the fact that the ER end effector is running off a 24V rail instead of 12V.
    if (ArmConfig::end_effector_type == ArmConfig::EE_EXTREME_RETRIEVAL && ArmConfig::wrist_type == ArmConfig::WRIST_CYCLOIDAL) {
        msg->end_effector_actuation *= 0.5;
    }
    end_effector->drive(msg->end_effector_actuation);

    // Linear actuator
    end_effector->set_linear_actuator(msg->linear_actuation);

}
// Reset the internal state
void ArmDriver::endeffector_input_deadline_callback()
{
    RCLCPP_WARN(this->get_logger(), "control/endeffector_input subscription deadline missed");
    // End effector
    end_effector->drive(0);
    // Linear actuator
    end_effector->set_linear_actuator(0);
}

ArmDriver::ArmDriver() : Node("arm_driver")
{
    // Creates the input subscription for the desired CMD commands (first 6 joints)
    rclcpp::SubscriptionOptionsWithAllocator<std::allocator<void>> joint_velocities_options;
    joint_velocities_options.event_callbacks.deadline_callback = [this](rclcpp::QOSDeadlineRequestedInfo) -> void {
        this->joint_velocities_deadline_callback();
    };
    joint_velocities_subscription = this->create_subscription<sensor_msgs::msg::JointState>(
        "/control/joint_velocities",
        rclcpp::QoS(1).best_effort().deadline(ROSTimers::arm_deadline),
        std::bind(&ArmDriver::joint_velocities_callback, this, _1),
        joint_velocities_options
    );
    
    // Creates the input subscription for the desired CMD commands (EE, LA)
    rclcpp::SubscriptionOptionsWithAllocator<std::allocator<void>> endeffector_input_options;
    endeffector_input_options.event_callbacks.deadline_callback = [this](rclcpp::QOSDeadlineRequestedInfo) -> void {
        this->endeffector_input_deadline_callback();
    };
    endeffector_input_subscription = this->create_subscription<core::msg::EndEffectorInput>(
        "/control/endeffector_input",
        rclcpp::QoS(1).best_effort().deadline(ROSTimers::arm_deadline),
        std::bind(&ArmDriver::endeffector_input_callback, this, _1),
        endeffector_input_options
    );


    // Initialise internal variabels

    // Arm model
    arm_model = new ArmModel(ArmConfig::wrist_type, ArmConfig::end_effector_type);
    // End effector
    end_effector = new CMD(1, 7, PWM, 1);

    
    // Output set-up messages
    Print::title("ARM DRIVER");
    Print::print("Subscribed Topics:");
    Print::print("/control/endeffector_input  [core/EndeffectorInput]", 1);
    Print::print("/control/joint_velocities   [sensor_msgs/JointState]", 1);
    Print::print("Published Topics:");
    Print::print("", true);
}

//  Main function called when the script execution begins
int main(int argc, char **argv)
{
    // Initialises the ROS C++ class
    rclcpp::init(argc, argv);

    // Runs the Publisher class
    rclcpp::spin(std::make_shared<ArmDriver>());

    // Shutsdown ROS once complete
    rclcpp::shutdown();

    // Returns an empty value
    return 0;
}
