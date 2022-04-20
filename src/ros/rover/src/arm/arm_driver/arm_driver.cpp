/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jess Hepworth
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the header file
#include "arm_driver.h"

#include "print/print.h"

#include "../hacky_defines.h"

// Receives the desired commands for the CMDs and sends to CMDs
void ArmDriver::joint_velocities_callback (const sensor_msgs::msg::JointState::SharedPtr msg)
{
    for (unsigned int i = 0; i < msg->name.size(); i++) {
        joints[i]->drive(msg->velocity[i]); 
    }
}
// Reset the internal velocities
void ArmDriver::joint_velocities_deadline_callback()
{
    RCLCPP_WARN(this->get_logger(), "control/joint_velocities subscription deadline missed");
    for (unsigned int i = 0; i < hack::JOINT_NAMES.size(); i++) {
        joints[i]->drive(0);
    }
}

// Receives the desired commands for the CMDs and sends to CMDs
void ArmDriver::endeffector_input_callback (const core::msg::EndEffectorInput::SharedPtr msg)
{
    // First 6 joints are handled by joint_velocities_callback

    // Receiving data for end effector
    joints[6]->drive(msg->end_effector_actuation); //need to create message

    // Linear actuator
    joints[6]->set_linear_actuator(msg->linear_actuation);

    // Lunar construction
    joints[7]->drive(msg->lunar_construction);
}
// Reset the internal state
void ArmDriver::endeffector_input_deadline_callback()
{
    RCLCPP_WARN(this->get_logger(), "control/endeffector_input subscription deadline missed");
    // End effector
    joints[6]->drive(0);
    // Linear actuator
    joints[6]->set_linear_actuator(0);
    // Lunar construction
    joints[7]->drive(0);
}

// Main constructor that sets up the node
ArmDriver::ArmDriver() : Node("arm_driver")
{    
    // Create joint instances based on the arm's structure
    // For now just hardcode for the cycloidal wrist and ES end effector
    // Eventually make this into a std::map and idenitfy particular joints based on their name instead of their position
    joints = std::vector<Joint*> (8);
    // Seventh CMD is end effector actuation, eighth is lunar construction
    CMD_drive_mode = std::vector<CMDCommand> {PID, PID, PID, PID, PID, PID, PWM, PWM};
    CMD_direction = std::vector<bool> {1, 1, 0, 0, 0, 0, 0, 0};

    for (unsigned int i = 0; i < joints.size(); i++) {
        joints[i] = new Joint (i + 1, CMD_drive_mode[i], CMD_direction[i]);
    }

    // Creates the input subscription for the desired CMD commands (first 6 joints)
    rclcpp::SubscriptionOptionsWithAllocator<std::allocator<void>> joint_velocities_options;
    joint_velocities_options.event_callbacks.deadline_callback = [this](rclcpp::QOSDeadlineRequestedInfo) -> void{
        this->joint_velocities_deadline_callback();
    };
    joint_velocities_subscription = this->create_subscription<sensor_msgs::msg::JointState>(
        "/control/joint_velocities",
        rclcpp::QoS(1).best_effort().deadline(200ms),
        std::bind(&ArmDriver::joint_velocities_callback, this, _1),
        joint_velocities_options
    );
    
    // Creates the input subscription for the desired CMD commands (LC, EE, LA)
    rclcpp::SubscriptionOptionsWithAllocator<std::allocator<void>> endeffector_input_options;
    endeffector_input_options.event_callbacks.deadline_callback = [this](rclcpp::QOSDeadlineRequestedInfo) -> void{
        thisendeffector();
    };
    endeffector_input_subscription = this->create_subscription<core::msg::EndEffectorInput>(
        "/control/endeffector_input",
        rclcpp::QoS(1).best_effort().deadline(200ms),
        std::bind(&ArmDriver::endeffector_input_callback, this, _1),
        endeffector_input_options
    );
    

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