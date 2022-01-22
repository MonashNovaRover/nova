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
EDITED:		 22/01/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include message types
#include "sensor_msgs/msg/joint_state.hpp"

// Include math constants and other libraries
#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>
#include <string>

// Arm configuration related defines
#define NUM_JOINTS 6


class ArmCore
{
    //------------------------------------------------------------//
    public:
    
    // List of names of all joints
    static std::vector<std::string> joint_names;
    
    // Model angles of each joint when the arm is in the resolver zeroing position
    // The arm is initialised from these angles, and then all angles are measured relative to that position
    static std::vector<double> zero_angles;

    // Construct an empty JointState message for use in ROS2 topics
    static sensor_msgs::msg::JointState get_empty_joint_state();

    // Parameters for arm model geometry. Based on model in Arm/DH parameters on GrabCAD
    // All distances in mm, all angles in rad
    // Lower joints
    constexpr static double SHOULDER_OFFSET = 124;  // Distance from p01 to p4 along z2
    constexpr static double ELBOW_LINK_LENGTH = 485;  // Distance from z2 to z3
    // Add intermediary frame here for vertical offset at elbow output. Would make angles easier later on

    // Cycloidal wrist
    constexpr static double J4_LINK_LENGTH = 499;  // Distance from z3 to z4 (not parallel to actual link)
    constexpr static double J5_OFFSET = 104;  // Distance from p4 to p56 along z5

    // ES Gripper end effector
    constexpr static double GRIPPER_OFFSET_Z = 311;  // Distance from p56 to tip of gripper end effector

    // Lower joints hook
    constexpr static double HOOK_OFFSET_X = -99;  // Distances from p4 to pE2 along axes xyzE2
    constexpr static double HOOK_OFFSET_Y = -24;
    constexpr static double HOOK_OFFSET_Z = 75;
    constexpr static double HOOK_ANGLE_X = -5.87 * M_PI/180;  // Angle from x3 to the axis of the cylindrical link


    // No constructor. No need to ever make an object of this class
};