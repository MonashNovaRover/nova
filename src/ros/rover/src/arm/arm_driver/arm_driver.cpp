/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jess Hepworth, Jory Braun
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

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

}
// Reset the internal state
void ArmDriver::endeffector_input_deadline_callback()
{
    RCLCPP_WARN(this->get_logger(), "control/endeffector_input subscription deadline missed");
    // End effector
    joints[6]->drive(0);
    // Linear actuator
    joints[6]->set_linear_actuator(0);
}

// Main constructor that sets up the node
ArmDriver::ArmDriver() : Node("arm_driver")
{    
    // Create joint instances based on the arm's structure
    // For now just hardcode for the cycloidal wrist and ES end effector
    // Eventually make this into a std::map and idenitfy particular joints based on their name instead of their position
    joints = std::vector<Joint*> (7);
    // Seventh CMD is end effector actuation
    CMD_drive_mode = std::vector<CMDCommand> {PID, PID, PID, PID, PID, PID, PWM};
    CMD_direction = std::vector<bool> {1, 1, 0, 0, 0, 0, 0};

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
    
    // Creates the input subscription for the desired CMD commands (EE, LA)
    rclcpp::SubscriptionOptionsWithAllocator<std::allocator<void>> endeffector_input_options;
    endeffector_input_options.event_callbacks.deadline_callback = [this](rclcpp::QOSDeadlineRequestedInfo) -> void{
        this->endeffector_input_deadline_callback();
    };
    endeffector_input_subscription = this->create_subscription<core::msg::EndEffectorInput>(
        "/control/endeffector_input",
        rclcpp::QoS(1).best_effort().deadline(200ms),
        std::bind(&ArmDriver::endeffector_input_callback, this, _1),
        endeffector_input_options
    );

    // Create the service client for arm_config_info
    arm_config_info_client = this->create_client<core::srv::ArmConfigInfo>("/control/arm_config_info");
    
    // Get the arm configuration info
    // Wait for the service to become available
    while (!arm_config_info_client->wait_for_service(1s)){
        RCLCPP_INFO(this->get_logger(), "Service /control/arm_config_info not available, waiting again...");
    }
    // Make the request
    /*
    auto arm_config_info_request = std::make_shared<core::srv::ArmConfigInfo::Request>();
    auto arm_config_info_response = arm_config_info_client->async_send_request(arm_config_info_request);
    // Wait for the result
    RCLCPP_WARN(this->get_logger(), "Started waiting for result");
    while (arm_config_info_response.wait_for(1s) != std::future_status::ready){
        RCLCPP_ERROR(this->get_logger(), "Failed to get response from /control/arm_config_info, waiting again...");
    }
    // Store the result
    arm_config_info = arm_config_info_response.get();
    RCLCPP_WARN(this->get_logger(), "Got result");
    */

    // Output set-up messages
    Print::title("ARM DRIVER");
    Print::print("Subscribed Topics:");
    Print::print("/control/endeffector_input  [core/EndeffectorInput]", 1);
    Print::print("/control/joint_velocities   [sensor_msgs/JointState]", 1);
    Print::print("Published Topics:");
    Print::print("Service Clients:");
    Print::print("/control/arm_config_info    [core/ArmConfigInfo]", 1);
    Print::print("", true);
}


//  Main function called when the script execution begins
int main(int argc, char **argv)
{
    // Initialises the ROS C++ class
    rclcpp::init(argc, argv);

    // Runs the Publisher class
    //rclcpp::spin(std::make_shared<ArmDriver>());

    std::shared_ptr<ArmDriver> node = std::make_shared<ArmDriver>();

    auto request = std::make_shared<core::srv::ArmConfigInfo::Request>();
    auto result = node->arm_config_info_client->async_send_request(request);
    // Wait for the result.
    if (rclcpp::spin_until_future_complete(node, result) ==
        rclcpp::executor::FutureReturnCode::SUCCESS)
    {
        RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "Got response!");
    } else {
        RCLCPP_ERROR(rclcpp::get_logger("rclcpp"), "Failed to call service");
    }

    // Shutsdown ROS once complete
    rclcpp::shutdown();

    // Returns an empty value
    return 0;
}