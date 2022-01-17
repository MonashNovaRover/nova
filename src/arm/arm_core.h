#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class manages all shared information associated
    with the configuration of the arm.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: None
TOPICS: None
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	 control
AUTHOR(S):   Jory Braun
CREATION:	 17/01/2022
EDITED:		 17/01/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Not sure if should include ROS messages here. Check it can build properly.
     Would be super helpful for avoiding code duplication
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include ROS client library
#include "rclcpp/rclcpp.hpp"
// Include message types
#include "sensor_msgs/msg/joint_state.hpp"

// Include math constants and other libraries
#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>
#include <string>

// Arm configuration related defines
#define NUM_JOINTS 6


class ArmCore : rclcpp::Node
{
    //------------------------------------------------------------//
    protected:
    
    // List of names of all joints
    const static std::vector<std::string> joint_names {"base-rotation", "shoulder", "elbow", "wrist-1", "wrist-2", "wrist-3"};
    
    // Model angles of each joint when the arm is in the resolver zeroing position
    // The arm is initialised from these angles, and then all angles are measured relative to that position
    const static std::vector<std::double> zero_angles {0, M_PI / 2, -M_PI / 2, 0, -M_PI / 2, 0};

    // Construct an empty JointState message for use in ROS2 topics
    static sensor_msgs::msg::JointState get_empty_joint_state()
    {
        // Create a joints message with the joint names
        sensor_msgs::msg::JointState joints();
        joints.name = joint_names;
        // Initialise state of the joints
        joints.position = zero_angles;
        joints.velocity = std::vector<double> (num_joints);
        joints.effort = std::vector<double> (num_joints);
    }

    // Parameters for arm model geometry. Based on model in Arm/DH parameters on GrabCAD
    // All distances in mm, all angles in rad
    // Lower joints
    enum LowerJointsModelGeometry
    {
        SHOULDER_OFFSET = 124,  // Distance from p01 to p4 along z2
        ELBOW_LINK_LENGTH = 485  // Distance from z2 to z3
        // Add intermediary frame here for vertical offset at elbow output. Would make angles easier later on
    };
    // Cycloidal wrist
    enum CycloidalWristModelGeometry
    {
        J4_LINK_LENGTH = 499,  // Distance from z3 to z4 (not parallel to actual link)
        J5_OFFSET = 104  // Distance from p4 to p56 along z5
    };
    // ES Gripper end effector
    enum ESGripperModelGeometry
    {
        GRIPPER_OFFSET_Z = 311  // Distance from p56 to tip of gripper end effector
    };
    // Lower joints hook
    enum HookModelGeometry
    {
        HOOK_OFFSET_X = -99,  // Distances from p4 to pE2 along axes xyzE2
        HOOK_OFFSET_Y = -24,
        HOOK_OFFSET_Z = 75,
        HOOK_ANGLE_X = 5.87 * M_PI/180  // Angle from x3 to the axis of the cylindrical link
    };

    //------------------------------------------------------------//
    public:

    // No constructor. No need to ever make an object of this class
}
