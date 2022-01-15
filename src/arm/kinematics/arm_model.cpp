/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jory Braun
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "arm_model.h"

#define _USE_MATH_DEFINES
#include <cmath>

ArmModel::ArmModel () : Node("arm_model")
{
    // Initialise constants
    // Will eventually be done in arm_core and then inherited here
    num_joints = 6;
    coord_frames_timer_period = 200ms;
    joint_velocities_timer_period = 200ms;
    joint_names = std::vector<std::string> {"base-rotation", "shoulder", "elbow", "wrist-1", "wrist-2", "wrist-3"};
    end_effector_names = std::vector<std::string> {"es-gripper", "er-gripper", "lc-gripper", "lower-joints-hook"};
    camera_names = std::vector<std::string> {"squooshy", "ee-front", "ee-depth", "ee-screw"};
    // Combine the 3 vectors above
    coord_frame_names = std::vector<std::string>();
    coord_frame_names.insert(coord_frame_names.end(), joint_names.begin(), joint_names.end());
    coord_frame_names.insert(coord_frame_names.end(), end_effector_names.begin(), end_effector_names.end());
    coord_frame_names.insert(coord_frame_names.end(), camera_names.begin(), camera_names.end());
    
    // Arm DH geometry constants (mm). Based on model in Arm/DH parameters on GrabCAD
    // Joints
    // DH parameter lengths
    double shoulder_offset = 124  // Distance from p01 to p4 along z2
    double elbow_link_length = 485  // Distance from z2 to z3
    double j4_link_length = 499  // Distance from z3 to z4 (not parallel to actual link)
    double j5_offset = 104  // Distance from p4 to p56 along z5
    // End effectors
    // Parameters for a general transformation from end effector to the previous joint
    double gripper_offset_z = 311  // Distance from p56 to tip of gripper end effector
    double hook_offset_x = -99  // Distances from p4 to pE2 along axes xyzE2
    double hook_offset_y = -24
    double hook_offset_z = 75
    double hook_angle_x = 5.87 * M_PI/180  // Angle from x3 to the axis of the cylindrical link (rad)
    // Joint initial angles
    zero_angles = std::vector<double> {0, 0, 0, 0, 0, 0};

    // Initialise arrays in internal data structures (TwistStamped does not need to be initialised)
    joints.name = joint_names;
    joints.position = std::vector<double> (num_joints);
    joints.velocity = std::vector<double> (num_joints);
    joints.effort = std::vector<double> (num_joints);

    coord_frames.joint_names = coord_frame_names;
    coord_frames.transforms = geometry_msgs::msg::Transform[num_joints];
    coord_frames.twist = geometry_msgs::msg::Twist[num_joints];
    coord_frames.wrench = geometry_msgs::msg::Wrench[num_joints];

    joint_velocities = joints;


    // Build the arm. Use the DH parameterisation
    // Create the joints
    // Joints
    KDL::Joint J1 = KDL::Joint(joint_names[0], KDL::Joint::RotZ);  // Base rotation
    KDL::Joint J2 = KDL::Joint(joint_names[1], KDL::Joint::RotZ);  // Shoulder
    KDL::Joint J3 = KDL::Joint(joint_names[2], KDL::Joint::RotZ);  // Elbow
    KDL::Joint J4 = KDL::Joint(joint_names[3], KDL::Joint::RotZ);  // Wrist 1
    KDL::Joint J5 = KDL::Joint(joint_names[4], KDL::Joint::RotZ);  // Wrist 2
    KDL::Joint J6 = KDL::Joint(joint_names[5], KDL::Joint::RotZ);  // Wrist 3
    // End effectors
    KDL::Joint E0 = KDL::Joint("es-gripper");
    KDL::Joint E1 = KDL::Joint("lower_joints_hook");

    // Create transformations between joints
    // Each one is a transformation from the current joint to the previous one
    // Joints
    // Use the modified DH parameters, so the origin of frame i is at the output of joint i
    KDL::Frame FJ1 = KDL::Frame::DH_Craig1989(0, 0, 0, zero_angles[0]);
    KDL::Frame FJ2 = KDL::Frame::DH_Craig1989(0, M_PI / 2, shoulder_offset, zero_angles[1]);
    KDL::Frame FJ3 = KDL::Frame::DH_Craig1989(elbow_link_length, 0, 0, zero_angles[2]);
    KDL::Frame FJ4 = KDL::Frame::DH_Craig1989(j4_link_length, 0, 0, zero_angles[3]);
    KDL::Frame FJ5 = KDL::Frame::DH_Craig1989(0, M_PI / -2, j5_offset, zero_angles[4]);
    KDL::Frame FJ6 = KDL::Frame::DH_Craig1989(0, M_PI / -2, 0, zero_angles[5]);
    // End effectors
    // ES gripper
    KDL::Frame FE0 = KDL::Frame(KDL::Vector(0, 0, gripper_offset_z));
    // Lower-joints hook
    KDL::Frame hook_to_j4 = KDL::Frame(KDL::Vector(hook_offset_x, hook_offset_y, hook_offset_z));
    KDL::Rotation j4_to_elbow_rot = KDL::Frame::Identity();
    j4_to_elbow_rot.DoRotZ(-M_PI);
    j4_to_elbow_rot.DoRotY(M_PI / 2);
    j4_to_elbow_rot.DoRotX(hook_angle_x);
    // Check this rotation matrix.
    // Check j4_to_elbow_rot, make sure rotation part matches with transformation_j4_to_elbow in old model.py 
    KDL::Frame j4_to_elbow = KDL::Frame(j4_to_elbow_rot, KDL::Vector(j4_link_length, 0, 0));

    // Create segments
    // Joints
    KDL::Segment SJ1 = KDL::Segment("SJ1", J1, FJ1);
    KDL::Segment SJ2 = KDL::Segment("SJ2", J2, FJ2);
    KDL::Segment SJ3 = KDL::Segment("SJ3", J3, FJ3);
    KDL::Segment SJ4 = KDL::Segment("SJ4", J4, FJ4);
    KDL::Segment SJ5 = KDL::Segment("SJ5", J5, FJ5);
    KDL::Segment SJ6 = KDL::Segment("SJ6", J6, FJ6);
    // End effectors
    KDL::Segment SE0 = KDL::Segment("SE0", E0, FE0);
    KDL::Segment SE1 = KDL::Segment("SE1", E1, FE1);

    // Create arm
    arm = KDL::Tree("root");
    arm.addSegment(SJ1, "root");
    arm.addSegment(SJ2, "SJ1");
    arm.addSegment(SJ3, "SJ2");
    arm.addSegment(SJ4, "SJ3");
    arm.addSegment(SJ5, "SJ4");
    arm.addSegment(SJ6, "SJ5");
    arm.addSegment(SE0, "SJ6");
    arm.addSegment(SE1, "SJ3");

    // Create FK solver
    arm_fk_solver = KDL::TreeFkSolverPos(arm);
    // Create IK solver
    //arm_ik_solver = KDL::TreeIkSolverVel(arm, WHAT GOES HERE);
    

    // Create subscription to resolvers
    resolver_sub = this->create_subscription<sensor_msgs::msg::JointState>(
        "/control/resolvers", 10, std::bind(&ArmModel::resolver_callback, this, -1)
    );

    // Create subscription to task_velocity
    task_velocity_sub = this->create_subscription<geometry_msgs::msg::TwistStamped>(
        "/control/task_velocity", 10, std::bind(&ArmModel::task_velocity_callback, this, _1)
    );

    // Create timer and publisher for arm_coord_frames
    coord_frames_timer = this->create_wall_timer(
        coord_frames_timer_period, std::bind(&ArmMode::publish_coord_frames, this)
    );
    coord_frames_pub = this->create_publisher<sensor_msgs::msg::MultiDOFJointState>(
        "/control/arm_coord_frames", 10
    );

    // Create timer and publisher for joint_velocities
    joint_velocities_timer = this->create_wall_timer(
        joint_velocities_timer_period, std::bind(&ArmMode::publish_joint_velocities, this)
    );
    joint_velocities_pub = this->create_publisher<sensor_msgs::msg::JointState>(
        "/control/joint_velocities", 10
    );
}


// Update the internal joint state
void resolver_callback(const sensor_msgs::msg::JointState::SharedPtr msg)
{
    joints = msg;
}

// Update the internal velocity
void task_velocity_callback(const geometrymsgs::msg::TwistStamped>::SharedPtr msg)
{
    task_velocity = msg;
}

// Update the arm model using the latest resolver info, publish to arm_cord_frames
void publish_coord_frames()
{
    // Get the input positions in the form KDL likes
    Eigen::VectorXd eigen_joints(joints.position.data());
    KDL::JntArray kdl_joints();
    kdl_joints.data = eigen_joints;
    // Prepare the output data structure
    KDL::Frame kdl_coord_frame();
    
    // Calculate FK for all joints
    // This is inefficient in KDL. For n joints takes O(n^2) time but could be O(n)
    for (int i = 0; i < coord_frames.transforms.size(); i++){
        // Calculate the FK for joint i. Store the result in kdl_coord_frame
        int exit_value = arm_fk_solver.JntToCart(
            kdl_joints, kdl_coord_frame, coord_frames.joint_names[i]
        );
        if (exit_value == -1){
            RCLCPP_WARN(this->get_logger(), "Number of positions provided does not match number of joints in tree");
        }
        else if (exit_value == -2){
            RCLCPP_WARN(this->get_logger(), "Could not find segment %s in the tree", coord_frames.joint_names[i].c_str());
        }
        else{
            // Success
            // Get the output transform in the form ROS2 likes
            geometry_msgs::msg::Vector3 translation();
            translation.x = kdl_coord_frame.p.x;
            translation.y = kdl_coord_frame.p.y;
            translation.z = kdl_coord_frame.p.z;
            geometry_msgs::msg::Quaternion rotation();
            kdl_coord_fram.M.GetQuaternion(rotation.x, rotation.y, rotation.z, rotation.w);
            geometry_msgs::msg::Transform transform();
            transform.translation = translation;
            transform.rotation = rotation;
            // Store the transform for this joint
            coord_frames.transforms[i] = transform;
        }
    }
    // Update the header
    coord_frames.header.stamp = this->now();
    // Publish the message
    coord_frames_pub->publish(coord_frames);
}

// Calculate the inverse kinematics using the latest arm model, publish to joint_velocities
void publish_joint_velocities()
{
    // Calculate the inverse kinematics
    //joint_velocities.velocity = MODEL.GET_THIS_BREAD(task_velocity);
    // Update the position too, since we have that info available
    joint_velocities.positions = joints.positions;
    // Update the header
    joint_velocities.header.stamp = this->now();
    // Publish the message
    //joint_velocities_pub->publish(joint_velocities);
}


//  Main function called when the script execution begins
int main(int argc, char **argv)
{
    // Initialises the ROS C++ class
    rclcpp::init(argc, argv);

    // Runs the Publisher class
    rclcpp::spin(std::make_shared<ArmModel>());

    // Shutsdown ROS once complete
    rclcpp::shutdown();

    // Returns an empty value
    return 0;
}