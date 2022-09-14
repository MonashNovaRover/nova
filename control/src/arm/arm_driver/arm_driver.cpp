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
    for (std::size_t i = 0; i < msg->name.size(); i++) {
        joints[i]->drive(msg->velocity[i]); 
    }
}
// Reset the internal velocities
void ArmDriver::joint_velocities_deadline_callback()
{
    RCLCPP_WARN(this->get_logger(), "control/joint_velocities subscription deadline missed");
    for (std::size_t i = 0; i < arm_config_info.joint_names.size(); i++) {
        joints[i]->drive(0);
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
    joints[6]->drive(msg->end_effector_actuation);

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

void ArmDriver::start_node()
{    
    // Create joint instances based on the arm's structure
    // For now just hardcode for the cycloidal wrist and ES end effector
    // Eventually make this into a std::map and idenitfy particular joints based on their name instead of their position
    double lower_joints_reduction = 2143.75;
    double wrist_reduction = 3002.499;
    int encoder_ppr = 512;
    double lower_joints_velocity_factor = 75;
    double wrist_velocity_factor = 50;
    double clock_frequency = 30e6;

    std::vector<CMD*> joints = {
        new CMD (1, 1, PID, STOP, 1, CMDOutputParameters(lower_joints_reduction, encoder_ppr, lower_joints_velocity_factor, clock_frequency)),  // J1
        new CMD (1, 2, PID, STOP, 1, CMDOutputParameters(lower_joints_reduction, encoder_ppr, lower_joints_velocity_factor, clock_frequency)),  // J2
        new CMD (1, 3, PID, STOP, 0, CMDOutputParameters(lower_joints_reduction, encoder_ppr, lower_joints_velocity_factor, clock_frequency)),  // J3
        new CMD (1, 4, PID, STOP, 0, CMDOutputParameters(wrist_reduction, encoder_ppr, wrist_velocity_factor, clock_frequency)),  // J4
        new CMD (1, 5, PID, STOP, 0, CMDOutputParameters(wrist_reduction, encoder_ppr, wrist_velocity_factor, clock_frequency)),  // J5
        new CMD (1, 6, PID, STOP, 0, CMDOutputParameters(wrist_reduction, encoder_ppr, wrist_velocity_factor, clock_frequency)),  // J6
        new CMD (1, 7, PWM)  // End effector
    };

    // Creates the input subscription for the desired CMD commands (first 6 joints)
    rclcpp::SubscriptionOptionsWithAllocator<std::allocator<void>> joint_velocities_options;
    joint_velocities_options.event_callbacks.deadline_callback = [this](rclcpp::QOSDeadlineRequestedInfo) -> void{
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
    endeffector_input_options.event_callbacks.deadline_callback = [this](rclcpp::QOSDeadlineRequestedInfo) -> void{
        this->endeffector_input_deadline_callback();
    };
    endeffector_input_subscription = this->create_subscription<core::msg::EndEffectorInput>(
        "/control/endeffector_input",
        rclcpp::QoS(1).best_effort().deadline(ROSTimers::arm_deadline),
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