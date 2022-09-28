/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jory Braun
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "arm_twistmapper.h"

#include "arm_messages.h"
#include "arm_type_translation.h"
#include "../arm_configuration.h"
#include "print/print.h"

#define _USE_MATH_DEFINES
#include <cmath>
#include <string>

ArmTwistMapper::ArmTwistMapper() : Node("arm_twist_mapper")
{    
    // Initialise publish timer periods
    task_velocity_timer_period = 50ms;
    
    
    // Create subscription to arm control scheme
    control_scheme_sub = this->create_subscription<core::msg::ArmControlScheme>(
        "/control/arm_control_scheme", 10, std::bind(&ArmTwistMapper::control_scheme_callback, this, _1)
    );

    // Create subscription to resolvers
    resolver_sub = this->create_subscription<sensor_msgs::msg::JointState>(
        "/electronics/resolvers", 10, std::bind(&ArmTwistMapper::resolver_callback, this, _1)
    );
    
    // Create subscription to input_task_velocity
    rclcpp::SubscriptionOptionsWithAllocator<std::allocator<void>> input_task_velocity_options;
    input_task_velocity_options.event_callbacks.deadline_callback = [this](rclcpp::QOSDeadlineRequestedInfo) -> void{
        this->input_task_velocity_deadline_callback();
    };
    input_task_velocity_sub = this->create_subscription<geometry_msgs::msg::TwistStamped>(
        "/control/input_task_velocity",
        rclcpp::QoS(1).best_effort().deadline(200ms),
        std::bind(&ArmTwistMapper::input_task_velocity_callback, this, _1),
        input_task_velocity_options
    );


    // Create timer and publisher for task_velocity
    task_velocity_timer = this->create_wall_timer(
        task_velocity_timer_period, std::bind(&ArmTwistMapper::publish_task_velocity, this)
    );
    task_velocity_pub = this->create_publisher<geometry_msgs::msg::TwistStamped>(
        "/control/task_velocity", rclcpp::QoS(1).best_effort().deadline(200ms)
    );


    // Initialise arm model and required solvers
    arm_model = new ArmModel(ArmConfig::wrist_type, ArmConfig::end_effector_type);
    arm_kinematics_solver = new ArmKinematics(*arm_model, this->get_logger());

    // Initialise arrays in internal data structures
    // Use data from the arm model
    joints = ArmMessages::get_empty_joint_state(arm_model->joint_names);


    // Output configuration messages
    // Convet module names to uppercase
    std::vector<std::string> module_names_upper = arm_model->module_names;
    for (auto& name : module_names_upper){
        for (auto& c : name){
            c = toupper(c);
        }
    }
    Print::title("ARM CONFIGURATION");
    Print::print("Wrist:");
    Print::print(module_names_upper[1].c_str(), 1);
    Print::print("End effector:");
    Print::print(module_names_upper[2].c_str(), 1);

    // Output set-up messages
    Print::title("ARM TWIST MAPPER");
    Print::print("Subscribed Topics:");
    Print::print("/control/arm_control_scheme       [core/ArmControlScheme]", 1);
    Print::print("/electronics/resolvers            [sensor_msgs/JointState]", 1);
    Print::print("/control/input_task_velocity      [geometry_msgs/TwistStamped]", 1);
    Print::print("Published Topics:");
    Print::print("/control/task_velocity            [geometry_msgs/TwistStamped]", 1);
    Print::print("", true);
}


// Update the internal control scheme
void ArmTwistMapper::control_scheme_callback(const core::msg::ArmControlScheme::SharedPtr msg)
{
    control_scheme = *msg;
}


// Update the internal joint positions
void ArmTwistMapper::resolver_callback(const sensor_msgs::msg::JointState::SharedPtr msg)
{
    joints = *msg;
}


// Update the internal task velocity
void ArmTwistMapper::input_task_velocity_callback(const geometry_msgs::msg::TwistStamped::SharedPtr msg)
{
    input_task_velocity = *msg;
}
// Reset the internal velocity
void ArmTwistMapper::input_task_velocity_deadline_callback()
{
    RCLCPP_WARN(this->get_logger(), "control/input_task_velocity subscription deadline missed");
    input_task_velocity = geometry_msgs::msg::TwistStamped();
}


// Get the twist from the joysticks
inline geometry_msgs::msg::Twist ArmTwistMapper::get_control_twist()
{
    // Unpack the ROS2 task velocity into KDL::Vectors
    KDL::Twist twist = ArmTypeTranslation::to_KDL_twist(task_velocity.twist);
    KDL::Vector& twist_linear = twist.vel;
    KDL::Vector& twist_angular = twist.rot;
    
    // Implement transformations on input linear and angular velocities
    // Endpoint frame control
    if (control_scheme.endpoint_frame_linear || control_scheme.endpoint_frame_angular){
        // Transform joystick input directions to end-effector coordinates
        // eg: forward on the left joystick is +ve x, but should be +ve z in end effector coordinates
        KDL::Rotation joystick_input_transform = KDL::Rotation::EulerZYX(M_PI / 2, -M_PI / 2, 0);
        // Transform from end effector coordinates to base frame coordinates
        KDL::JntArray joint_positions = ArmTypeTranslation::to_KDL_jnt_array(joints.position);
        KDL::Rotation endpoint_frame_transform = arm_kinematics_solver->fk_pos_end_effector(joint_positions).M;
        if (control_scheme.endpoint_frame_linear) {
            twist_linear = endpoint_frame_transform * joystick_input_transform * twist_linear;
        }
        if (control_scheme.endpoint_frame_angular) {
            // Add additional transform to switch yaw and roll directions for more intuitive control
            joystick_input_transform = KDL::Rotation::EulerZYX(0, 0, M_PI / 2) * joystick_input_transform;
            twist_angular = endpoint_frame_transform * joystick_input_transform * twist_angular;
        }
    }
    // Reference frame offset
    if (control_scheme.base_frame_offset != 0){
        KDL::Rotation base_offset_transform = KDL::Rotation::RotZ(M_PI / 2 * control_scheme.base_frame_offset);
        // Apply offset only if endpoint-frame control not applied
        if (!control_scheme.endpoint_frame_linear){
            twist_linear = base_offset_transform * twist_linear;
        }
        if (!control_scheme.endpoint_frame_angular) {
            twist_angular = base_offset_transform * twist_angular;
        }
    }

    // Return the twist
    return ArmTypeTranslation::to_ROS2_twist(twist);
}


// Calculate the inverse kinematics using the latest arm model, publish to joint_velocities
void ArmTwistMapper::publish_task_velocity()
{
    // Get the twist in the rover frame
    task_velocity.twist = get_control_twist();

    // Update the header
    task_velocity.header.stamp = this->now();
    // Publish the message
    task_velocity_pub->publish(task_velocity);
}


//  Main function called when the script execution begins
int main(int argc, char **argv)
{
    // Initialises the ROS C++ class
    rclcpp::init(argc, argv);

    // Runs the Publisher class
    rclcpp::spin(std::make_shared<ArmTwistMapper>());

    // Shutsdown ROS once complete
    rclcpp::shutdown();

    // Returns an empty value
    return 0;
}
