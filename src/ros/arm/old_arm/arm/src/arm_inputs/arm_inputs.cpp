/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jess Hepworth, Jory Braun, Matthew Gu
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the header file
#include "arm_inputs/arm_inputs.h"

#include "arm_core/arm_messages.h"
#include "colors.h"
#include "rosconfig.h"
#include <iostream>

// Use the standard namespaces
using std::placeholders::_1;


ArmInputs::ArmInputs() : ArmConfigInfoClient("arm_inputs"), joystick_override(true) { }

InputDevice* ArmInputs::select_input_device(){
    if (joystick_override) {
        if (joystick_translate.is_connected()) {
            return &joystick_translate;
        } else {
            return &keyboard_translate;
        }
    } else {
        if (keyboard_translate.is_connected()) {
            return &keyboard_translate;
        } else {
            return &joystick_translate;
        }
    }
}

// Receives input from left joystick
void ArmInputs::joystick_l_callback (const input_interfaces::msg::InputJoystick::SharedPtr msg)
{
    // Save data for later, only deal with it when we publish
    // More efficient, works if we only care about the most up-to-date message
    joystick_translate.joystick_l_callback(msg);
}

// Receives input from right joystick
void ArmInputs::joystick_r_callback (const input_interfaces::msg::InputJoystick::SharedPtr msg)
{
    // Save data for later, only deal with it when we publish
    // More efficient, works if we only care about the most up-to-date message    
    joystick_translate.joystick_r_callback(msg);
}

void ArmInputs::keyboard_callback(const input_interfaces::msg::InputKeyboard::SharedPtr msg)
{
    keyboard_translate.keyboard_callback(msg);   
}

// Resets joystick internal state
void ArmInputs::joystick_deadline_callback()
{
    RCLCPP_WARN(this->get_logger(), "Joystick subscriber deadline missed");
    joystick_translate.reset_message();
}

void ArmInputs::keyboard_deadline_callback()
{   
    RCLCPP_WARN(this->get_logger(), "Keyboard subscriber deadline missed");
    keyboard_translate.reset_message();
}

// Publishes data on the arm input
void ArmInputs::publish_endeffector_inputs ()
{
    // Get output from device
    select_input_device()->get_end_effector_inputs(control_scheme, end_effector_inputs);

    // Publish the arm inputs
    endeffector_pub->publish(end_effector_inputs);
}

// Publishes joint velocity data
void ArmInputs::publish_joint_velocities ()
{   
    // Get output from device
    select_input_device()->get_joint_velocity_inputs(control_scheme, joint_velocities);

    // Set the header
    joint_velocities.header.stamp = this->now();
    // Publish the joint space velocities
    joint_velocities_pub->publish(joint_velocities);
}

// Publishes task velocity data
void ArmInputs::publish_twist ()
{
    // Get output from device
    select_input_device()->get_twist_inputs(control_scheme, twist);

    // Set the header
    twist.header.stamp = this->now();
    // Publish the joint space velocities
    twist_pub->publish(twist);
}

// Publishes control data
void ArmInputs::publish_inputs()
{
    publish_joint_velocities();
    publish_twist();
}

// Publishes control scheme data
void ArmInputs::publish_control_scheme()
{   
    // Get output from device
    if(select_input_device()->get_control_scheme_inputs(control_scheme)) {
        joystick_override = !joystick_override;
        std::cout << C_MODE << "Input device swicthed: " << (joystick_override ? "Joystick" : "Keyboard") << C_END << std::endl;
        if (joystick_override){
            std::cout << C_MODE << "Press 'Bottom right button on right side of left joystick' to switch back" << C_END << std::endl;
        } else {
            std::cout << C_MODE << "Press 'ctrl + 0' to switch back" << C_END << std::endl;
        }
    }

    // Publish the control scheme
    control_scheme.header.stamp = this->now();
    control_scheme_pub->publish(control_scheme);
}


void ArmInputs::start_node()
{
    // Create common options for joystick subscriptions
    rclcpp::SubscriptionOptionsWithAllocator<std::allocator<void>> joystick_options;
    joystick_options.event_callbacks.deadline_callback = [this](rclcpp::QOSDeadlineRequestedInfo) -> void {
        this->joystick_deadline_callback();
    };
    // Options for keyboard subscription
    rclcpp::SubscriptionOptionsWithAllocator<std::allocator<void>> keyboard_options;
    keyboard_options.event_callbacks.deadline_callback = [this](rclcpp::QOSDeadlineRequestedInfo) -> void {
        this->keyboard_deadline_callback();
    };

    // QoS options for arm input subscriptions
    rclcpp::QoS arm_input_qos = rclcpp::QoS(1).best_effort().deadline(ROSTimers::arm_deadline);

    // Creates the input subscription for the left joystick (with QoS options)
    joystick_l_sub = this->create_subscription<input_interfaces::msg::InputJoystick>(
        "/inputs/input_joystick_l",
        arm_input_qos,
        std::bind(&ArmInputs::joystick_l_callback, this, _1),
        joystick_options
    );

    // Creates the input subscription for the right joystick (with QoS options)
    joystick_r_sub = this->create_subscription<input_interfaces::msg::InputJoystick>(
        "/inputs/input_joystick_r",
        arm_input_qos,
        std::bind(&ArmInputs::joystick_r_callback, this, _1),
        joystick_options
    );

    // Creates the input subscription for the keyboard (with QoS options)
    keyboard_sub = this->create_subscription<input_interfaces::msg::InputKeyboard>(
        "/inputs/input_keyboard",
        arm_input_qos,
        std::bind(&ArmInputs::keyboard_callback, this, _1),
        keyboard_options
    );

    // Create timer and publisher for endeffector_inputs
    endeffector_pub_timer = this->create_wall_timer(
        ROSTimers::arm_control, std::bind(&ArmInputs::publish_endeffector_inputs, this)
    );
    endeffector_pub = this->create_publisher<arm_interfaces::msg::EndEffectorInput>(
        "/arm/endeffector_input", rclcpp::QoS(1).best_effort().deadline(ROSTimers::arm_deadline)
    );

    // Create timer and publisher for joystick_joint_velocities and joystick_twist
    inputs_pub_timer = this->create_wall_timer(
        ROSTimers::arm_control, std::bind(&ArmInputs::publish_inputs, this)
    );
    joint_velocities_pub = this->create_publisher<sensor_msgs::msg::JointState>(
        "/arm/joystick_joint_velocities", rclcpp::QoS(1).best_effort().deadline(ROSTimers::arm_deadline)
    );
    twist_pub = this->create_publisher<geometry_msgs::msg::TwistStamped>(
        "/arm/joystick_twist", rclcpp::QoS(1).best_effort().deadline(ROSTimers::arm_deadline)
    );

    // Create timer and publisher for control_scheme
    control_scheme_pub_timer = this->create_wall_timer(
        ROSTimers::arm_control, std::bind(&ArmInputs::publish_control_scheme, this)
    );    
    control_scheme_pub = this->create_publisher<arm_interfaces::msg::ArmControlScheme>(
        "/arm/arm_control_scheme", 10
    );

    // Initialise arrays in internal data structures
    joint_velocities = ArmMessages::get_empty_joint_state(arm_config_info.joint_names_6dof);
    
    // Publish the control scheme to initialise other nodes
    // Uses the default field values
    publish_control_scheme();

    // Output set-up messages
    std::cout << C_TITLE << "ARM INPUTS" << C_END << "\n";
    std::cout << "Subscribed Topics:\n";
    std::cout << "/inputs/input_joystick_l            [input_interfaces/InputJoystick]\n";
    std::cout << "/inputs/input_joystick_r            [input_interfaces/InputJoystick]\n";
    std::cout << "Published Topics:\n";
    std::cout << "/arm/endeffector_input           [arm_interfaces/EndEffectorInput]\n";
    std::cout << "/arm/joystick_joint_velocities   [sensor_msgs/JointState]\n";
    std::cout << "/arm/joystick_twist              [sensor_msgs/TwistStamped]\n";
    std::cout << "/arm/arm_control_scheme          [arm_interfaces/ArmControlScheme]\n" << std::endl;
}


//  Main function called when the script execution begins
int main(int argc, char **argv)
{
    // Initialises the ROS C++ class
    rclcpp::init(argc, argv);

    // Runs the Publisher class
    rclcpp::spin(std::make_shared<ArmInputs>());

    // Shutsdown ROS once complete
    rclcpp::shutdown();

    // Returns an empty value
    return 0;
}
