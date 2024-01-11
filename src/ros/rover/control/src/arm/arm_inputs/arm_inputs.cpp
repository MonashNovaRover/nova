/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jess Hepworth, Jory Braun, Matthew Gu
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the header file
#include "arm_inputs.h"

#include "arm_messages.h"
#include "print/print.h"
#include "config/rosconfig.h"

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
void ArmInputs::joystick_l_callback (const core::msg::InputJoystick::SharedPtr msg)
{
    // Save data for later, only deal with it when we publish
    // More efficient, works if we only care about the most up-to-date message
    joystick_translate.joystick_l_callback(msg);
}

// Receives input from right joystick
void ArmInputs::joystick_r_callback (const core::msg::InputJoystick::SharedPtr msg)
{
    // Save data for later, only deal with it when we publish
    // More efficient, works if we only care about the most up-to-date message    
    joystick_translate.joystick_r_callback(msg);
}

void ArmInputs::keyboard_callback(const core::msg::InputKeyboard::SharedPtr msg)
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
    // Create a new message
    auto message = core::msg::EndEffectorInput();

    // Get output from device
    CommonInputCollections::EndEffectorInputs end_effector_inputs = select_input_device()->get_end_effector_inputs();

    message.linear_actuation = end_effector_inputs.linear_actuation;
    message.end_effector_actuation = end_effector_inputs.end_effector_actuation;
    
    // Publish the arm inputs
    endeffector_pub->publish(message);
}

// Publishes joint velocity data
void ArmInputs::publish_joint_velocities ()
{   
    CommonInputCollections::JointVelocityInputs velocities = select_input_device()->get_joint_velocity_inputs();

    for (long unsigned int i = 0; i < sizeof(velocities.velocities)/sizeof(velocities.velocities[0]); i++) {
        joint_velocities.velocity[i] = velocities.velocities[i];
    }

    // Set the header
    joint_velocities.header.stamp = this->now();
    // Publish the joint space velocities
    joint_velocities_pub->publish(joint_velocities);
}

// Publishes task velocity data
void ArmInputs::publish_twist ()
{
    CommonInputCollections::TwistInputs twist_inputs = select_input_device()->get_twist_inputs();

    twist.twist.linear.x = twist_inputs.linear.x;
    twist.twist.linear.y = twist_inputs.linear.y;
    twist.twist.linear.z = twist_inputs.linear.z;

    twist.twist.angular.x = twist_inputs.angular.x;
    twist.twist.angular.y = twist_inputs.angular.y;
    twist.twist.angular.z = twist_inputs.angular.z;

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

    CommonInputCollections::ControlSchemeInputs control_scheme_inputs = select_input_device()->get_control_scheme_inputs();

    // Set button-based data here so we don't miss any button-press events
    control_scheme.input_lock = control_scheme_inputs.input_lock;
    control_scheme.joint_limits = control_scheme_inputs.joint_limits;
#if POSITION_CONTROL_ENABLE
    // Position control
    control_scheme.position_control = control_scheme_inputs.position_control;
#endif
    control_scheme.base_frame_offset = control_scheme_inputs.base_frame_offset;
    control_scheme.flat_frame_linear = control_scheme_inputs.flat_frame_linear;
    control_scheme.flat_frame_angular = control_scheme_inputs.flat_frame_angular;
    control_scheme.endpoint_frame_linear = control_scheme_inputs.endpoint_frame_linear;
    control_scheme.endpoint_frame_angular = control_scheme_inputs.endpoint_frame_angular;
    control_scheme.ik_linear = control_scheme_inputs.ik_linear;
    control_scheme.ik_angular = control_scheme_inputs.ik_angular;
    control_scheme.use_spm_roll = control_scheme_inputs.use_spm_roll;
    control_scheme.zero_resolvers = control_scheme_inputs.zero_resolvers;

    // Correction for position control - can't have independent linear and angular control
    if (control_scheme.position_control) {
        control_scheme.flat_frame_angular = control_scheme.flat_frame_linear;
        control_scheme.endpoint_frame_angular = control_scheme.endpoint_frame_linear;
        control_scheme.ik_angular = control_scheme.ik_linear;
    }
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
    joystick_l_sub = this->create_subscription<core::msg::InputJoystick>(
        "/control/input_joystick_l",
        arm_input_qos,
        std::bind(&ArmInputs::joystick_l_callback, this, _1),
        joystick_options
    );

    // Creates the input subscription for the right joystick (with QoS options)
    joystick_r_sub = this->create_subscription<core::msg::InputJoystick>(
        "/control/input_joystick_r",
        arm_input_qos,
        std::bind(&ArmInputs::joystick_r_callback, this, _1),
        joystick_options
    );

    // Creates the input subscription for the keyboard (with QoS options)
    keyboard_sub = this->create_subscription<core::msg::InputKeyboard>(
        "/control/input_keyboard",
        arm_input_qos,
        std::bind(&ArmInputs::keyboard_callback, this, _1),
        keyboard_options
    );

    // Create timer and publisher for endeffector_inputs
    endeffector_pub_timer = this->create_wall_timer(
        ROSTimers::arm_control, std::bind(&ArmInputs::publish_endeffector_inputs, this)
    );
    endeffector_pub = this->create_publisher<core::msg::EndEffectorInput>(
        "/control/endeffector_input", rclcpp::QoS(1).best_effort().deadline(ROSTimers::arm_deadline)
    );

    // Create timer and publisher for joystick_joint_velocities and joystick_twist
    inputs_pub_timer = this->create_wall_timer(
        ROSTimers::arm_control, std::bind(&ArmInputs::publish_inputs, this)
    );
    joint_velocities_pub = this->create_publisher<sensor_msgs::msg::JointState>(
        "/control/joystick_joint_velocities", rclcpp::QoS(1).best_effort().deadline(ROSTimers::arm_deadline)
    );
    twist_pub = this->create_publisher<geometry_msgs::msg::TwistStamped>(
        "/control/joystick_twist", rclcpp::QoS(1).best_effort().deadline(ROSTimers::arm_deadline)
    );

    // Create timer and publisher for control_scheme
    control_scheme_pub_timer = this->create_wall_timer(
        ROSTimers::arm_control, std::bind(&ArmInputs::publish_control_scheme, this)
    );    
    control_scheme_pub = this->create_publisher<core::msg::ArmControlScheme>(
        "/control/arm_control_scheme", 10
    );

    // Initialise arrays in internal data structures
    joint_velocities = ArmMessages::get_empty_joint_state(arm_config_info.joint_names_6dof);
    
    // Publish the control scheme to initialise other nodes
    // Uses the default field values
    publish_control_scheme();

    // Output set-up messages
    Print::title("ARM INPUTS");
    Print::print("Subscribed Topics:");
    Print::print("/control/input_joystick_l            [core/InputJoystick]", 1);
    Print::print("/control/input_joystick_r            [core/InputJoystick]", 1);
    Print::print("/control/input_keyboard              [core/InputKeyboard]", 1);
    Print::print("Published Topics:");
    Print::print("/control/endeffector_input           [core/EndEffectorInput]", 1);
    Print::print("/control/joystick_joint_velocities   [sensor_msgs/JointState]", 1);
    Print::print("/control/joystick_twist              [sensor_msgs/TwistStamped]", 1);
    Print::print("/control/arm_control_scheme          [core/ArmControlScheme]", 1);
    Print::print("", true);
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
