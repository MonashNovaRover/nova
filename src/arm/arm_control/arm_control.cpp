/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jory Braun
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "arm_control.h"

#include "arm_messages.h"
#include "arm_type_translation.h"
#include "../arm_configuration.h"
#include "print/print.h"

#include <string>

ArmControl::ArmControl() : Node("arm_control")
{
    // Initialise publish timer periods
    coord_frames_timer_period = 200ms;
    joint_velocities_timer_period = 50ms;
    

    // Create subscription to arm control scheme
    control_scheme_sub = this->create_subscription<core::msg::ArmControlScheme>(
        "/control/arm_control_scheme", 10, std::bind(&ArmControl::control_scheme_callback, this, _1)
    );
    
    // Create subscription to resolvers
    resolver_sub = this->create_subscription<sensor_msgs::msg::JointState>(
        "/electronics/resolvers", 10, std::bind(&ArmControl::resolver_callback, this, _1)
    );

    // Create subscription to input joint velocities
    rclcpp::SubscriptionOptionsWithAllocator<std::allocator<void>> input_joint_velocities_options;
    input_joint_velocities_options.event_callbacks.deadline_callback = [this](rclcpp::QOSDeadlineRequestedInfo) -> void{
        this->input_joint_velocities_deadline_callback();
    };
    input_joint_velocities_sub = this->create_subscription<sensor_msgs::msg::JointState>(
        "/control/input_joint_velocities",
        rclcpp::QoS(1).best_effort().deadline(200ms),
        std::bind(&ArmControl::input_joint_velocities_callback, this, _1),
        input_joint_velocities_options
    );
    
    // Create subscription to task_velocity
    rclcpp::SubscriptionOptionsWithAllocator<std::allocator<void>> task_velocity_options;
    task_velocity_options.event_callbacks.deadline_callback = [this](rclcpp::QOSDeadlineRequestedInfo) -> void{
        this->task_velocity_deadline_callback();
    };
    task_velocity_sub = this->create_subscription<geometry_msgs::msg::TwistStamped>(
        "/control/task_velocity",
        rclcpp::QoS(1).best_effort().deadline(200ms),
        std::bind(&ArmControl::task_velocity_callback, this, _1),
        task_velocity_options
    );

    // Create subscription to task_position
    rclcpp::SubscriptionOptionsWithAllocator<std::allocator<void>> task_position_options;
    task_position_options.event_callbacks.deadline_callback = [this](rclcpp::QOSDeadlineRequestedInfo) -> void{
        this->task_position_deadline_callback();
    };
    task_position_sub = this->create_subscription<geometry_msgs::msg::TransformStamped>(
        "/control/task_position",
        rclcpp::QoS(1).best_effort().deadline(200ms),
        std::bind(&ArmControl::task_position_callback, this, _1),
        task_position_options
    );

    // Create timer and publisher for arm_coord_frames
    coord_frames_timer = this->create_wall_timer(
        coord_frames_timer_period, std::bind(&ArmControl::publish_coord_frames, this)
    );
    coord_frames_pub = this->create_publisher<sensor_msgs::msg::MultiDOFJointState>(
        "/control/arm_coord_frames", 10
    );

    // Create timer and publisher for joint_velocities
    joint_velocities_timer = this->create_wall_timer(
        joint_velocities_timer_period, std::bind(&ArmControl::publish_joint_velocities, this)
    );
    joint_velocities_pub = this->create_publisher<sensor_msgs::msg::JointState>(
        "/control/joint_velocities", rclcpp::QoS(1).best_effort().deadline(200ms)
    );


    // Create service for arm_config_info
    arm_config_info_service = this->create_service<core::srv::ArmConfigInfo>(
        "/control/arm_config_info", std::bind(&ArmControl::arm_config_info_callback, this, _1, _2)
    );


    // Initialise internal variables

    // Arm model and solvers
    arm_model = new ArmModel(ArmConfig::wrist_type, ArmConfig::end_effector_type);
    arm_kinematics_solver = new ArmKinematics(*arm_model, this->get_logger());

    // Controllers for each joint
    for (uint16_t i = 0; i < arm_model->num_joints; i++) {
        ArmSubModule::ControlCoeffs coeffs = arm_model->control_coeffs[i];
        controllers.push_back(new PIController(coeffs.prop, coeffs.integral));
    }

    // Timestep calculation
    prev_time = this->now();

    // Arrays in internal data structures
    // Use data from the arm model
    joints = ArmMessages::get_empty_joint_state(arm_model->joint_names);
    joint_space_input = ArmMessages::get_empty_joint_state(arm_model->joint_names);
    coord_frames = ArmMessages::get_empty_multi_dof_joint_state(arm_model->segment_names);


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
    Print::title("ARM CONTROL");
    Print::print("Subscribed Topics:");
    Print::print("/control/arm_control_scheme       [core/ArmControlScheme]", 1);
    Print::print("/electronics/resolvers            [sensor_msgs/JointState]", 1);
    Print::print("/control/input_joint_velocities   [sensor_msgs/JointState]", 1);
    Print::print("/control/task_velocity            [geometry_msgs/TwistStamped]", 1);
    Print::print("/control/task_position            [geometry_msgs/TransformStamped]", 1);
    Print::print("Published Topics:");
    Print::print("/control/arm_coord_frames         [sensor_msgs/MultiDOFJointState]", 1);
    Print::print("/control/joint_velocities         [sensor_msgs/JointState]", 1);
    Print::print("Services:");
    Print::print("/control/arm_config_info          [core/ArmConfigInfo]", 1);
    Print::print("", true);
}

// Update the internal control scheme
void ArmControl::control_scheme_callback(const core::msg::ArmControlScheme::SharedPtr msg)
{
    control_scheme = *msg;
}

// Update the internal joint positions
void ArmControl::resolver_callback(const sensor_msgs::msg::JointState::SharedPtr msg)
{
    joints = *msg;
}

// Update the internal joint-space joint velocities
void ArmControl::input_joint_velocities_callback(const sensor_msgs::msg::JointState::SharedPtr msg)
{
    joint_space_input = *msg;
}
// Reset the internal velocities
void ArmControl::input_joint_velocities_deadline_callback()
{
    RCLCPP_WARN(this->get_logger(), "control/input_joint_velocities subscription deadline missed");
    joint_space_input = ArmMessages::get_empty_joint_state(arm_model->joint_names);
}

// Update the internal task velocity
void ArmControl::task_velocity_callback(const geometry_msgs::msg::TwistStamped::SharedPtr msg)
{
    task_velocity = *msg;
}
// Reset the internal velocity
void ArmControl::task_velocity_deadline_callback()
{
    RCLCPP_WARN(this->get_logger(), "control/task_velocity subscription deadline missed");
    task_velocity = geometry_msgs::msg::TwistStamped();
}

// Update the internal task position
void ArmControl::task_position_callback(const geometry_msgs::msg::TransformStamped::SharedPtr msg)
{
    task_position = *msg;
}
// Reset the internal position
void ArmControl::task_position_deadline_callback()
{
    RCLCPP_WARN(this->get_logger(), "control/task_position subscription deadline missed");
    task_position = geometry_msgs::msg::TransformStamped();
}


// Update the arm model using the latest resolver info, publish to arm_cord_frames
void ArmControl::publish_coord_frames()
{
    // Calculate the forward kineamtics for all segments
    KDL::JntArray joint_positions = ArmTypeTranslation::to_KDL_jnt_array(joints.position);
    std::vector<KDL::Frame> kdl_frames = arm_kinematics_solver->fk_pos_all_segments(joint_positions);

    // Get the coord frames in the form ROS2 likes
    for (uint16_t i = 0; i < arm_model->num_segments; i++) {
        coord_frames.transforms[i] = ArmTypeTranslation::to_ROS2_transform(kdl_frames[i]);
    }

    // Update the header
    coord_frames.header.stamp = this->now();
    // Publish the message
    coord_frames_pub->publish(coord_frames);
}


// Calculate the control error
inline KDL::Twist ArmControl::get_control_error(const KDL::Frame& control_pose, const KDL::Frame& end_effector_pose)
{
    KDL::Twist error;

    // Position error
    if (control_scheme.position_control_linear) {
        error.vel = control_pose.p - end_effector_pose.p;
    }
    else {
        KDL::SetToZero(error.vel);
    }

    // Orientation error
    if (control_scheme.position_control_angular) {
        KDL::Rotation error_transform = control_pose.M * end_effector_pose.M.Inverse();
        error.rot = error_transform.GetRot();
    }
    else {
        KDL::SetToZero(error.rot);
    }

    // Prevent error discontinuities from causing the arm to make large movements
    if (error.vel.Norm() > ERROR_LIMIT_LINEAR || error.rot.Norm() > ERROR_LIMIT_ANGULAR) {
        KDL::SetToZero(error);
        RCLCPP_WARN(this->get_logger(), "Large control error detected. Switch to velocity control or reset position control.");
    }

    return error;
}


// Get the joint-space velocities of all joints on the arm using inverse kinematics
inline KDL::JntArray ArmControl::get_joint_velocities(double timestep)
{
    // Get the inputs to the control loop
    KDL::JntArray joint_positions = ArmTypeTranslation::to_KDL_jnt_array(joints.position);
    KDL::JntArray control_joint_velocities = ArmTypeTranslation::to_KDL_jnt_array(joint_space_input.position);
    KDL::Twist control_twist = ArmTypeTranslation::to_KDL_twist(task_velocity.twist);
    KDL::Frame control_pose = ArmTypeTranslation::to_KDL_frame(task_position.transform);


    // Calculate the feedforward velocity term

    // Get the control twist into the joint space of the 6-DOF serial model
    KDL::JntArray feedforward_joint_velocities = arm_kinematics_solver->ik_vel_end_effector_6dof(joint_positions, control_twist);
    // Add the 6-DOF joint velocities
    feedforward_joint_velocities.data += control_joint_velocities.data;
    // Convert to joint space
    feedforward_joint_velocities = arm_kinematics_solver->joint_vel_transform_6dof_to_actual(joint_positions, feedforward_joint_velocities, control_scheme.use_spm_roll);


    // Calculate the position loop

    // Get the control error, accoutning for the control scheme
    KDL::Frame end_effector_pose = arm_kinematics_solver->fk_pos_end_effector(joint_positions);
    KDL::Twist control_error = get_control_error(control_pose, end_effector_pose);
    // Convert to joint space
    KDL::JntArray feedback_joint_velocities = arm_kinematics_solver->ik_vel_end_effector(joint_positions, control_error, control_scheme.use_spm_roll);
    // Apply controllers
    if (control_error != KDL::Twist::Zero()) {
        double* error;
        for (uint16_t i = 0; i < arm_model->num_joints; i++) {
            error = &feedback_joint_velocities.data[i];
            *error = controllers[i]->update(*error, timestep);
        }
    }


    // Get the ideal output velocities
    KDL::JntArray joint_velocities;
    joint_velocities.data = feedforward_joint_velocities.data + feedback_joint_velocities.data;


    // Apply saturation to each joint
    bool joint_limited = false;
    double velocity_multiplier = 1;
    for (uint16_t i = 0; i < arm_model->num_joints; i++) {

        // Joint limits
        if (control_scheme.joint_limits &&
            ((joints.position[i] >= arm_model->joint_limits[i].upper && joint_velocities.data[i] > 0)
            || (joints.position[i] <= arm_model->joint_limits[i].lower && joint_velocities.data[i] < 0))){

            controllers[i]->saturated = true;
            RCLCPP_WARN(this->get_logger(), "Joint %s has reached a limit", joints.name[i].c_str());
            joint_limited = true;
        }

        else{
            // Maximum speed
            double speed = abs(joint_velocities.data[i]);
            double max_speed = arm_model->drivers[i]->max_speed;
            if (speed > max_speed) {
                controllers[i]->saturated = true;
                RCLCPP_WARN(this->get_logger(), "Joint %s has reached maximum velocity", joints.name[i].c_str());
                velocity_multiplier = max_speed / speed;
            }

            // Not saturated
            else {
                controllers[i]->saturated = false;
            }
        }
    }

    // If saturated, modify joint velocities
    if (joint_limited) {
        KDL::SetToZero(joint_velocities);
    }
    else if (velocity_multiplier != 1) {
        joint_velocities.data *= velocity_multiplier;
    }

    // Retuen the joint velocities
    return joint_velocities;
}


// Calculate the inverse kinematics using the latest arm model, publish to joint_velocities
void ArmControl::publish_joint_velocities()
{
    // Calculate the inverse kinematics and update the commanded joint velocities
    rclcpp::Time current_time = this->now();
    double timestep = (current_time - prev_time).seconds();
    KDL::JntArray joint_velocities = get_joint_velocities(timestep);
    prev_time = current_time;

    // Fill the output message
    joints.velocity = std::vector<double> (joint_velocities.data.data(), joint_velocities.data.data() + joint_velocities.data.size());

    // Update the header
    joints.header.stamp = current_time;
    // Publish the message
    joint_velocities_pub->publish(joints);
}


// Return details of the arm model
void ArmControl::arm_config_info_callback(
    __attribute__((unused)) const core::srv::ArmConfigInfo::Request::SharedPtr request,
    core::srv::ArmConfigInfo::Response::SharedPtr response
)
{
    // Store names of relevant model features
    response->module_names = arm_model->module_names;
    response->joint_names = arm_model->joint_names;
    response->endpoint_names = arm_model->endpoint_names;
    response->default_endpoint_name = arm_model->default_endpoint_name;
    response->segment_names = arm_model->segment_names;

    // Store joint limits
    std::vector<float> joint_limits_lower (arm_model->num_joints);
    std::vector<float> joint_limits_upper (arm_model->num_joints);
    for (uint16_t i = 0; i < arm_model->num_joints; i++) {
        joint_limits_lower[i] = arm_model->joint_limits[i].lower;
        joint_limits_upper[i] = arm_model->joint_limits[i].upper;
    }
    response->joint_limits_lower = joint_limits_lower;
    response->joint_limits_upper = joint_limits_upper;

    // Store number of joints and segments
    response->num_joints = arm_model->num_joints;
    response->num_segments = arm_model->num_segments;
}


//  Main function called when the script execution begins
int main(int argc, char **argv)
{
    // Initialises the ROS C++ class
    rclcpp::init(argc, argv);

    // Runs the Publisher class
    rclcpp::spin(std::make_shared<ArmControl>());

    // Shutsdown ROS once complete
    rclcpp::shutdown();

    // Returns an empty value
    return 0;
}
