/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jory Braun
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "arm_kinematics.h"

#include <Eigen/Core>

#include "arm_messages.h"
#include "../arm_configuration.h"
#include "print/print.h"

#define _USE_MATH_DEFINES
#include <cmath>
#include <string>

ArmKinematics::ArmKinematics() : Node("arm_kinematics")
{
    // Initialise constants
    coord_frames_timer_period = 200ms;
    joint_velocities_timer_period = 50ms;

    // Initialise arm model
    arm_model = new ArmModel(ArmConfig::wrist_type, ArmConfig::end_effector_type);
    // Initialise arm kinematics solvers
    spm_solver = new SpmKinematics();
    serial_fk_solver = new KDL::TreeFkSolverPos_recursive(*arm_model);
    serial_ik_solver = new KDL::TreeIkSolverVel_wdls(*arm_model, std::vector<std::string> {arm_model->default_endpoint_name});

    // Initialise arrays in internal data structures
    // Use data from the arm model
    joints = ArmMessages::get_empty_joint_state(arm_model->joint_names);
    joint_space_input = ArmMessages::get_empty_joint_state(arm_model->joint_names);
    // TwistStamped does not need to be initialised
    coord_frames = ArmMessages::get_empty_multi_dof_joint_state(arm_model->segment_names);
    
    // Create subscription to arm control scheme
    control_scheme_sub = this->create_subscription<core::msg::ArmControlScheme>(
        "/control/arm_control_scheme", 10, std::bind(&ArmKinematics::control_scheme_callback, this, _1)
    );
    
    // Create subscription to resolvers
    resolver_sub = this->create_subscription<sensor_msgs::msg::JointState>(
        "/electronics/resolvers", 10, std::bind(&ArmKinematics::resolver_callback, this, _1)
    );

    // Create subscription to input joint velocities
    rclcpp::SubscriptionOptionsWithAllocator<std::allocator<void>> input_joint_velocities_options;
    input_joint_velocities_options.event_callbacks.deadline_callback = [this](rclcpp::QOSDeadlineRequestedInfo) -> void{
        this->input_joint_velocities_deadline_callback();
    };
    input_joint_velocities_sub = this->create_subscription<sensor_msgs::msg::JointState>(
        "/control/input_joint_velocities",
        rclcpp::QoS(1).best_effort().deadline(200ms),
        std::bind(&ArmKinematics::input_joint_velocities_callback, this, _1),
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
        std::bind(&ArmKinematics::task_velocity_callback, this, _1),
        task_velocity_options
    );

    // Create timer and publisher for arm_coord_frames
    coord_frames_timer = this->create_wall_timer(
        coord_frames_timer_period, std::bind(&ArmKinematics::publish_coord_frames, this)
    );
    coord_frames_pub = this->create_publisher<sensor_msgs::msg::MultiDOFJointState>(
        "/control/arm_coord_frames", 10
    );

    // Create timer and publisher for joint_velocities
    joint_velocities_timer = this->create_wall_timer(
        joint_velocities_timer_period, std::bind(&ArmKinematics::publish_joint_velocities, this)
    );
    joint_velocities_pub = this->create_publisher<sensor_msgs::msg::JointState>(
        "/control/joint_velocities", rclcpp::QoS(1).best_effort().deadline(200ms)
    );

    // Create service for arm_config_info
    arm_config_info_service = this->create_service<core::srv::ArmConfigInfo>(
        "/control/arm_config_info", std::bind(&ArmKinematics::arm_config_info_callback, this, _1, _2)
    );

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
    Print::title("ARM KINEMATICS");
    Print::print("Subscribed Topics:");
    Print::print("/electronics/resolvers            [sensor_msgs/JointState]", 1);
    Print::print("/control/input_joint_velocities   [sensor_msgs/JointState]", 1);
    Print::print("/control/task_velocity            [geometry_msgs/TwistStamped]", 1);
    Print::print("Published Topics:");
    Print::print("/control/arm_coord_frames         [sensor_msgs/MultiDOFJointState]", 1);
    Print::print("/control/joint_velocities         [sensor_msgs/JointState]", 1);
    Print::print("Services:");
    Print::print("/control/arm_config_info          [core/ArmConfigInfo]", 1);
    Print::print("", true);
}

// Update the internal control scheme
void ArmKinematics::control_scheme_callback(const core::msg::ArmControlScheme::SharedPtr msg)
{
    control_scheme = *msg;
}

// Update the internal joint positions
void ArmKinematics::resolver_callback(const sensor_msgs::msg::JointState::SharedPtr msg)
{
    joints = *msg;
}

// Update the internal joint-space joint velocities
void ArmKinematics::input_joint_velocities_callback(const sensor_msgs::msg::JointState::SharedPtr msg)
{
    joint_space_input = *msg;
}
// Reset the internal velocities
void ArmKinematics::input_joint_velocities_deadline_callback()
{
    RCLCPP_WARN(this->get_logger(), "control/input_joint_velocities subscription deadline missed");
    joint_space_input = ArmMessages::get_empty_joint_state(arm_model->joint_names);
}

// Update the internal task velocity
void ArmKinematics::task_velocity_callback(const geometry_msgs::msg::TwistStamped::SharedPtr msg)
{
    task_velocity = *msg;
}
// Reset the internal velocity
void ArmKinematics::task_velocity_deadline_callback()
{
    RCLCPP_WARN(this->get_logger(), "control/task_velocity subscription deadline missed");
    task_velocity = geometry_msgs::msg::TwistStamped();
}

// Get the joint-space positions of the serial model of the arm
inline KDL::JntArray ArmKinematics::get_serial_joint_positions()
{
    KDL::JntArray kdl_joints;
    // Get the data directly from the resolvers
    kdl_joints.data = Eigen::Matrix<double, 6, 1> (joints.position.data());
    
    // If using the SPM wrist, replace SPM input joint positions with equivalent serial pitch, yaw and roll
    if (ArmConfig::wrist_type == ArmConfig::WRIST_SPM){
        // Calculate SPM FK
        std::vector<double> spm_joints (joints.position.begin() + 3, joints.position.begin() + 6); 
        std::vector<double> serial_wrist_joints = spm_solver->spm_fk(spm_joints);
        // Pitch
        kdl_joints.data[3] = serial_wrist_joints[0];
        // Yaw
        kdl_joints.data[4] = serial_wrist_joints[1];
        // Roll. Combine SPM roll and end-rotation since the KDL model requires 6 joints
        kdl_joints.data[5] = serial_wrist_joints[2] + joints.position[6];
    }

    return kdl_joints;
}

// Calculate the FK for a given segment
inline KDL::Frame ArmKinematics::calculate_serial_fk(KDL::JntArray kdl_joints, std::string segment_name)
{
    // Prepare the output data structure
    KDL::Frame kdl_coord_frame = KDL::Frame::Identity();
    
    // Calculate the FK for the given segment. Store the result in kdl_coord_frame
    int exit_value = serial_fk_solver->JntToCart(kdl_joints, kdl_coord_frame, segment_name);
    if (exit_value == -1){
        RCLCPP_WARN(this->get_logger(), "Number of positions provided does not match number of joints in tree");
    }
    else if (exit_value == -2){
        RCLCPP_WARN(this->get_logger(), "Could not find segment %s in the tree", segment_name.c_str());
    }

    return kdl_coord_frame;
}

// Get the task-space positions of all coordinate frames on the arm using forward kinematics
inline void ArmKinematics::update_coord_frames()
{
    // Get the input positions for the serial model of the arm, accounting for the SPM wrist
    KDL::JntArray kdl_joints = get_serial_joint_positions();

    // Calculate FK for all joints
    // This is inefficient in KDL. For n joints takes O(n^2) time but could be O(n)
    for (std::size_t i = 0; i < coord_frames.transforms.size(); i++){
        // Calculate the FK for joint i. Store the result in kdl_coord_frame
        KDL::Frame kdl_coord_frame = calculate_serial_fk(kdl_joints, coord_frames.joint_names[i]);
        
        // Save the output transform in the form ROS2 likes
        geometry_msgs::msg::Vector3 translation;
        translation.x = kdl_coord_frame.p.x();
        translation.y = kdl_coord_frame.p.y();
        translation.z = kdl_coord_frame.p.z();
        geometry_msgs::msg::Quaternion rotation;
        kdl_coord_frame.M.GetQuaternion(rotation.x, rotation.y, rotation.z, rotation.w);
        geometry_msgs::msg::Transform transform;
        transform.translation = translation;
        transform.rotation = rotation;
        // Store the transform for this joint
        coord_frames.transforms[i] = transform;
    }
}

// Update the arm model using the latest resolver info, publish to arm_cord_frames
void ArmKinematics::publish_coord_frames()
{
    // Calculate the forward kineamtics
    update_coord_frames();

    // Update the header
    coord_frames.header.stamp = this->now();
    // Publish the message
    coord_frames_pub->publish(coord_frames);
}

// Get the twist from the joysticks
inline KDL::Twist ArmKinematics::get_control_twist()
{
    // Unpack the ROS2 task velocity into KDL::Vectors
    const geometry_msgs::msg::Vector3& vec3_linear = task_velocity.twist.linear;
    const geometry_msgs::msg::Vector3& vec3_angular = task_velocity.twist.angular;
    KDL::Vector twist_linear (vec3_linear.x, vec3_linear.y, vec3_linear.z);
    KDL::Vector twist_angular (vec3_angular.x, vec3_angular.y, vec3_angular.z);
    
    // Implement transformations on input linear and angular velocities
    // Endpoint frame control
    if (control_scheme.endpoint_frame_linear || control_scheme.endpoint_frame_angular){
        // Transform joystick input directions to end-effector coordinates
        // eg: forward on the left joystick is +ve x, but should be +ve z in end effector coordinates
        KDL::Rotation joystick_input_transform = KDL::Rotation::EulerZYX(M_PI / 2, -M_PI / 2, 0);
        // Transform from end effector coordinates to base frame coordinates
        KDL::Rotation endpoint_frame_transform = calculate_serial_fk(get_serial_joint_positions(), arm_model->default_endpoint_name).M;
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

    // Compose into final twist
    return KDL::Twist (twist_linear, twist_angular);
}

// Solve the velocity inverse kineamtics for the end effector
inline KDL::JntArray ArmKinematics::calculate_serial_ik(KDL::JntArray kdl_joint_positions, KDL::Twist kdl_twist)
{
    // Get the input twist in the form KDL likes
    KDL::Twists kdl_twists = { {arm_model->default_endpoint_name, kdl_twist} };
    
    // Prepare the output data structure
    KDL::JntArray kdl_joint_velocities (6);
    
    // Calculate the inverse kinematics. Store the result in kdl_joint_velocities
    double exit_value = serial_ik_solver->CartToJnt(kdl_joint_positions, kdl_twists, kdl_joint_velocities);
    if (exit_value == -1){
        RCLCPP_WARN(this->get_logger(), "Must provide 6 positions and have 6 joints in tree");
    }
    else if (exit_value == -2){
        RCLCPP_WARN(this->get_logger(), "Twists provided must have a corresponding endpoint which is a segment in the tree");
    }
    else if (exit_value == KDL::TreeIkSolverVel_wdls::E_SVD_FAILED) {
        RCLCPP_WARN(this->get_logger(), "Singular value decomposition failed");
    }

    return kdl_joint_velocities;
}

// Get the joint-space velocities of all joints on the arm for the given task velocity using inverse kinematics
inline void ArmKinematics::update_joint_velocities()
{
    // Calculate IK for the end effector
    // Gets the joint velocities for the serial model of the arm
    KDL::Twist kdl_twist = get_control_twist();
    KDL::JntArray kdl_joint_velocities (6);
    if (kdl_twist != KDL::Twist::Zero()){
        kdl_joint_velocities = calculate_serial_ik(get_serial_joint_positions(), kdl_twist);
    }

    // Combine the IK and joint-space joint velocities for the serial model of the arm
    // Save the output in the form ROS2 likes
    for (unsigned int i = 0; i < 6; i++) {
        joints.velocity[i] = kdl_joint_velocities.data[i] + joint_space_input.velocity[i];
    }

    // If using the SPM wrist, replace serial pitch, yaw and roll with SPM input joint velocities
    if (ArmConfig::wrist_type == ArmConfig::WRIST_SPM){
        
        // If using end rotation instead of SPM roll, move the serial roll to the index for end rotation
        // Then no roll will be passed to the SPM IK
        if (!control_scheme.use_spm_roll){
            joints.velocity[6] = joints.velocity[5];
            joints.velocity[5] = 0;
        }

        // Calculate SPM IK
        std::vector<double> spm_joints (joints.position.begin() + 3, joints.position.begin() + 6);
        std::vector<double> serial_wrist_velocity (joints.velocity.begin() + 3, joints.velocity.begin() + 6);
        std::vector<double> spm_velocity = spm_solver->spm_ik_velocity(spm_joints, serial_wrist_velocity);
        // Replace serial pitch, yaw and roll with SPM input joint velocities
        // Pitch
        joints.velocity[3] = spm_velocity[0];
        // Yaw
        joints.velocity[4] = spm_velocity[1];
        // Roll
        joints.velocity[5] = spm_velocity[2];
    }

    // If activated, apply joint limits to the current joint velocity
    if (control_scheme.joint_limits){
        // If any joint hits a limit, stop all joints
        bool limit = false;
        for (std::size_t i = 0; i < joints.name.size(); i++) {
            if ((joints.position[i] >= arm_model->joint_limits[i].upper && joints.velocity[i] > 0)
                || (joints.position[i] <= arm_model->joint_limits[i].lower && joints.velocity[i] < 0)){
                RCLCPP_WARN(this->get_logger(), "Joint %s has reached a limit", joints.name[i].c_str());
                limit = true;
            }
        }
        if (limit) {
            // Clear the velocity data
            std::fill(joints.velocity.begin(), joints.velocity.end(), 0);
        }
    }
}

// Calculate the inverse kinematics using the latest arm model, publish to joint_velocities
void ArmKinematics::publish_joint_velocities()
{
    // Calculate the inverse kinematics and update the commanded joint velocities
    update_joint_velocities();

    // Update the header
    joints.header.stamp = this->now();
    // Publish the message
    joint_velocities_pub->publish(joints);
}

// Return details of the arm model
void ArmKinematics::arm_config_info_callback(
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
    std::vector<float> joint_limits_lower (arm_model->joint_limits.size());
    std::vector<float> joint_limits_upper (arm_model->joint_limits.size());
    for (std::size_t i = 0; i < arm_model->joint_limits.size(); i++) {
        joint_limits_lower[i] = arm_model->joint_limits[i].lower;
        joint_limits_upper[i] = arm_model->joint_limits[i].upper;
    }
    response->joint_limits_lower = joint_limits_lower;
    response->joint_limits_upper = joint_limits_upper;
}

//  Main function called when the script execution begins
int main(int argc, char **argv)
{
    // Initialises the ROS C++ class
    rclcpp::init(argc, argv);

    // Runs the Publisher class
    rclcpp::spin(std::make_shared<ArmKinematics>());

    // Shutsdown ROS once complete
    rclcpp::shutdown();

    // Returns an empty value
    return 0;
}
