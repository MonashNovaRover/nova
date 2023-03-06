/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Manika Goyal
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the header file
#include "scraper_inputs.h"

#include "scraper_messages.h"
#include "print/print.h"
#include "config/rosconfig.h"

// Use the standard namespaces
using std::placeholders::_1;


// Receives input from left joystick
void ExcavationConstructionInputs::joystick_l_callback (const core::msg::InputJoystick::SharedPtr msg)
{
    // Save data for later, only deal with it when we publish
    // More efficient, works if we only care about the most up-to-date message
    joystick_l = *msg;

    // Set button-based data here so we don't miss any button-press events
    bool control_scheme_update = false;
    // Scraper lock
    if (joystick_l.btn_bottom_l2_state == 1) {
        if (!control_scheme.joystick_lock)
            Print::print("Joysticks locked");
        control_scheme.joystick_lock = true;
        control_scheme_update = true;
    }
    if (joystick_l.btn_bottom_l5_state == 1){
        if (control_scheme.joystick_lock)
            Print::print("Joysticks Unlocked");
        control_scheme.joystick_lock = false;
        control_scheme_update = true;
    }
#endif
    // Immediately publish any new control scheme data
    // Also will continue to publish when the timer expires
    if (control_scheme_update){
        publish_control_scheme();
    }

    if (joystick_l.btn_thumb_l_state == 1) {
        if (!control_scheme.tile_placer_mode)
            Print::print("Tile Placer Mode On");
        control_scheme.joystick_lock = true;
        control_scheme_update = true;
    }
    if (joystick_l.btn_thumb_l_state == 1){
        if (control_scheme.tile_placer_modes)
            Print::print("Tile Placer Mode Off");
        control_scheme.joystick_lock = false;
        control_scheme_update = true;
    }
#endif
    // Immediately publish any new control scheme data
    // Also will continue to publish when the timer expires
    if (control_scheme_update){
        publish_control_scheme();
    } 
    btn_thumb_l_state

}



// Receives input from right joystick
void ExcavationConstructionInputs::joystick_r_callback (const core::msg::InputJoystick::SharedPtr msg)
{
    // Save data for later, only deal with it when we publish
    // More efficient, works if we only care about the most up-to-date message    
    joystick_r = *msg;
}

// Resets joystick internal state
void ExcavationConstructionInputs::joystick_deadline_callback()
{
    RCLCPP_WARN(this->get_logger(), "Joystick subscriber deadline missed");
    joystick_l = core::msg::InputJoystick();
    joystick_r = core::msg::InputJoystick();
}

// Publishes data on the arm input
void ExcavationConstructionInputs::publish_endeffector_inputs ()
{
    // Create a new message
    auto message = core::msg::EndEffectorInput();

    if (!control_scheme.joystick_lock){
        // Set the values for linear actuator and end effector actuation
        message.linear_actuation = joystick_l.ax_thumb_x;
        message.end_effector_actuation = calculate_direction(joystick_r.ax_thumb_x) * 0.95;
    }
    
    // Publish the arm inputs
    endeffector_pub->publish(message);
}

// Publishes joint velocity data
void ExcavationConstructionInputs::publish_scraper_arm_velocity ()
{
    // Get the speed from slider, apply scaling
    float speed = scale_speed(joystick_r.ax_slider);
    
    // If using lower joints joint-space control
    if (!control_scheme.joystick_lock) {        
        
        // Scraper arm is stick x (forward-backward). Forward pitches scraper arm down
        scraper_arm_velocity = speed * -joystick_l.ax_stick_x;
    }
    else{
        scraper_arm_velocity = 0;
    }

    // Set the header
    joint_velocities.header.stamp = this->now();
    // Publish the joint space velocities
    scraper_arm_velocity_pub->publish(scraper_arm_velocity);
}

// Publishes joint velocity data
void ExcavationConstructionInputs::publish_scraper_scoop_velocity ()
{
    // Get the speed from slider, apply scaling
    float speed = scale_speed(joystick_r.ax_slider);
    
    // If using lower joints joint-space control
    if (!control_scheme.joystick_lock) {        
        
        // Scraper arm is stick x (forward-backward). Forward pitches scraper arm down
        scraper_scoop_velocity = speed * -joystick_r.ax_stick_x;
    }
    else{
        scraper_scoop_velocity = 0;
    }

    // Set the header
    joint_velocities.header.stamp = this->now();
    // Publish the joint space velocities
    scraper_scoop_velocity_pub->publish(scraper_scoop_velocity);
}

// Publishes joint velocity data
void ExcavationConstructionInputs::publish_tile_placer_velocity ()
{
    // Get the speed from slider, apply scaling
    float speed = scale_speed(joystick_r.ax_slider);
    
    // If using lower joints joint-space control
    if (!control_scheme.joystick_lock) {        
        
        // Scraper arm is stick x (forward-backward). Forward pitches scraper arm down
        scraper_scoop_velocity = speed * -joystick_r.ax_stick_x;
    }
    else{
        scraper_scoop_velocity = 0;
    }

    // Set the header
    joint_velocities.header.stamp = this->now();
    // Publish the joint space velocities
    scraper_scoop_velocity_pub->publish(scraper_scoop_velocity);
}

float ExcavationConstructionInputs::calculate_direction (float value){
    if (value > 0){
        return 1.0;
    }
    else if (value < 0){
        return -1.0;
    }
    else{
        return 0.0;
    }
}


float ExcavationConstructionInputs::scale_speed (float value){
    // Max scale factor 1.00, min scale factor 0.05
    return (value * 0.95) + 0.05;
}

// Publishes control data
void ExcavationConstructionInputs::publish_inputs()
{
    publish_tile_placer_velocity();
    publish_scraper_arm_velocity();
    publish_scraper_scoop_velocity();
}

// Publishes control scheme data
void ExcavationConstructionInputs::publish_control_scheme()
{   
    // Buttons are handled separately
    
    // Set base reference frame offset
    int8_t base_frame_offset = 0;
    if (joystick_l.ax_slider < 0.3) {
        base_frame_offset = -1;
    }
    else if (joystick_l.ax_slider > 0.8) {
        base_frame_offset = 1;
    }
    control_scheme.base_frame_offset = base_frame_offset;

    // Control schemes
    // Endpoint frame control. Hold trigger
    control_scheme.endpoint_frame_linear = joystick_l.btn_thumb_u_state == 2;
    control_scheme.endpoint_frame_angular = joystick_r.btn_thumb_u_state == 2;
    // IK. Hold inside thumb button.
    // Also set if endpoint frame control is used.
    control_scheme.ik_linear = joystick_l.btn_thumb_r_state == 2 || control_scheme.endpoint_frame_linear;
    control_scheme.ik_angular = joystick_r.btn_thumb_l_state == 2 || control_scheme.endpoint_frame_angular;
    // Set SPM roll handling. Hold back thumb button on right stick
    control_scheme.use_spm_roll = joystick_r.btn_thumb_d_state == 2;

    // Correction for position control - can't have independent linear and angular control
    if (control_scheme.position_control) {
        control_scheme.endpoint_frame_angular = control_scheme.endpoint_frame_linear;
        control_scheme.ik_angular = control_scheme.ik_linear;
    }

    // Set the header and publish
    control_scheme.header.stamp = this->now();
    control_scheme_pub->publish(control_scheme);
}


void ExcavationConstructionInputs::start_node()
{
    // Create common options for joystick subscriptions
    rclcpp::SubscriptionOptionsWithAllocator<std::allocator<void>> joystick_options;
    joystick_options.event_callbacks.deadline_callback = [this](rclcpp::QOSDeadlineRequestedInfo) -> void{
        this->joystick_deadline_callback();
    };
    rclcpp::QoS joystick_qos = rclcpp::QoS(1).best_effort().deadline(ROSTimers::arm_deadline);

    // Creates the input subscription for the left joystick (with QoS options)
    joystick_l_sub = this->create_subscription<core::msg::InputJoystick>(
        "/control/input_joystick_l",
        joystick_qos,
        std::bind(&ExcavationConstructionInputs::joystick_l_callback, this, _1),
        joystick_options
    );

    // Creates the input subscription for the right joystick (with QoS options)
    joystick_r_sub = this->create_subscription<core::msg::InputJoystick>(
        "/control/input_joystick_r",
        joystick_qos,
        std::bind(&ExcavationConstructionInputs::joystick_r_callback, this, _1),
        joystick_options
    );

    // Create timer and publisher for endeffector_inputs
    endeffector_pub_timer = this->create_wall_timer(
        ROSTimers::arm_control, std::bind(&ExcavationConstructionInputs::publish_endeffector_inputs, this)
    );
    endeffector_pub = this->create_publisher<core::msg::EndEffectorInput>(
        "/control/endeffector_input", rclcpp::QoS(1).best_effort().deadline(ROSTimers::arm_deadline)
    );

    // Create timer and publisher for joystick_joint_velocities and joystick_twist
    inputs_pub_timer = this->create_wall_timer(
        ROSTimers::arm_control, std::bind(&ExcavationConstructionInputs::publish_inputs, this)
    );
    joint_velocities_pub = this->create_publisher<sensor_msgs::msg::JointState>(
        "/control/joystick_joint_velocities", rclcpp::QoS(1).best_effort().deadline(ROSTimers::arm_deadline)
    );
    twist_pub = this->create_publisher<geometry_msgs::msg::TwistStamped>(
        "/control/joystick_twist", rclcpp::QoS(1).best_effort().deadline(ROSTimers::arm_deadline)
    );

    // Create timer and publisher for control_scheme
    control_scheme_pub_timer = this->create_wall_timer(
        ROSTimers::arm_control, std::bind(&ExcavationConstructionInputs::publish_control_scheme, this)
    );    
    control_scheme_pub = this->create_publisher<core::msg::ExcavationConstructionControlScheme>(
        "/control/arm_control_scheme", 10
    );

    // Initialise arrays in internal data structures
    joint_velocities = ExcavationConstructionMessages::get_empty_joint_state(arm_config_info.joint_names_6dof);
    
    // Publish the control scheme to initialise other nodes
    // Uses the default field values
    publish_control_scheme();

    // Output set-up messages
    Print::title("ARM INPUTS");
    Print::print("Subscribed Topics:");
    Print::print("/control/input_joystick_l            [core/InputJoystick]", 1);
    Print::print("/control/input_joystick_r            [core/InputJoystick]", 1);
    Print::print("Published Topics:");
    Print::print("/control/endeffector_input           [core/EndEffectorInput]", 1);
    Print::print("/control/joystick_joint_velocities   [sensor_msgs/JointState]", 1);
    Print::print("/control/joystick_twist              [sensor_msgs/TwistStamped]", 1);
    Print::print("/control/arm_control_scheme          [core/ExcavationConstructionControlScheme]", 1);
    Print::print("", true);
}


//  Main function called when the script execution begins
int main(int argc, char **argv)
{
    // Initialises the ROS C++ class
    rclcpp::init(argc, argv);

    // Runs the Publisher class
    rclcpp::spin(std::make_shared<ExcavationConstructionInputs>());

    // Shutsdown ROS once complete
    rclcpp::shutdown();

    // Returns an empty value
    return 0;
}
