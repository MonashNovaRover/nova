/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jory Braun
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "arm_core.h"

// Initialise non-const static members of ArmCore
int num_arm_nodes = 0;

std::vector<std::string> ArmCore::joint_names = {
    "base-rotation",
    "shoulder",
    "elbow",
    "wrist-1",
    "wrist-2",
    "wrist-3"
    };

std::vector<std::string> ArmCore::end_effector_names = {
    "es-gripper",
    "er-gripper",
    "lc-gripper",
    "lower-joints-hook"
    };

std::vector<std::string> ArmCore::camera_names = {
    "squooshy",
    "ee-front",
    "ee-depth",
    "ee-screw"
    };

// Hardcode this for now, but make modular later
std::vector<std::string> ArmCore::coord_frame_names = {
    "base-rotation",
    "shoulder",
    "elbow",
    "wrist-1",
    "wrist-2",
    "wrist-3",
    "es-gripper",
    "er-gripper",
    "lc-gripper",
    "lower-joints-hook",
    "squooshy",
    "ee-front",
    "ee-depth",
    "ee-screw"
};

std::vector<double> ArmCore::zero_angles = {
    0,
    M_PI / 2,
    -M_PI / 2,
    0,
    -M_PI / 2,
    0
    };

// Get empty MultiDOFJointState for use in other nodes and topics
sensor_msgs::msg::MultiDOFJointState ArmCore::get_empty_multi_dof_joint_state()
{
    sensor_msgs::msg::MultiDOFJointState coord_frames;
    coord_frames.joint_names = ArmCore::coord_frame_names;
    coord_frames.transforms = std::vector<geometry_msgs::msg::Transform> (NUM_JOINTS);
    coord_frames.twist = std::vector<geometry_msgs::msg::Twist> (NUM_JOINTS);
    coord_frames.wrench = std::vector<geometry_msgs::msg::Wrench> (NUM_JOINTS);
    return coord_frames;
}

// Get empty JointState for use in other nodes and topics
sensor_msgs::msg::JointState ArmCore::get_empty_joint_state()
{
    sensor_msgs::msg::JointState joints;
    joints.name = ArmCore::joint_names;
    joints.position = ArmCore::zero_angles;
    joints.velocity = std::vector<double> (NUM_JOINTS);
    joints.effort = std::vector<double> (NUM_JOINTS);
    return joints;
}
