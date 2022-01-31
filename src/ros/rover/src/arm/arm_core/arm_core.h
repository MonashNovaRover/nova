#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

These classes manage all shared information associated
  with the configuration of the arm.
Every arm node inherits one of these classes, and information is
  shared through ROS to keep every node up-to-date.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: None
TOPICS:
  - /control/arm_params       [core/ArmParams]        [Subscribed]
  - /control/arm_params       [core/ArmParams]        [Published]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	 control
AUTHOR(S):   Jory Braun
CREATION:	 17/01/2022
EDITED:		 01/02/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Create ArmParams message type. Store a copy of it in ArmCore
 - Implement ArmCoreSubscriber
 - Implement ArmCorePublisher
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include ROS client library
#include "rclcpp.hpp"
// Include message types
#include "sensor_msgs/msg/joint_state.hpp"
#include "sensor_msgs/msg/multi_dof_joint_state.hpp"

// Include other libraries
#include <vector>
#include <string>

// Arm configuration related defines
// Eventually remove and use dynamic joint_names, model_joint_names, etc
#define NUM_JOINTS 6


class ArmCore : public rclcpp::Node
{  
    //------------------------------------------------------------//
    protected:
    
    // List names of all joints and all control points (cameras and tips of end effectors)
    // Initialised by arm_kinematics when the arm is built
    // Separate real joints (used for resolver / motor driving things) and modelled joints (used for FK / IK)
    // Real and modelled joints would usually be the same, but the SPM wirst has an extra roll DOF which is not used.
    std::vector<std::string> joint_names;
    std::vector<std::string> model_joint_names;
    std::vector<std::string> control_point_names;

    /// @brief  Helper function to construct empty JointState message
    ///         Uses given names of joints, sizes all other parameter lists to match
    sensor_msgs::msg::JointState get_empty_joint_state(const std::vector<std::string>& names);

    /// @brief  Helper function to construct empty MultiDOFJointState message
    ///         Uses given names of joints, sizes all other parameter lists to match
    sensor_msgs::msg::MultiDOFJointState get_empty_multi_dof_joint_state(const std::vector<std::string>& names);

    /// @brief  Constructor. Starts the node with the given name, sets up core publisher and subscriber
    ///         Make it protected so this class cannot be instantiated
    ArmCore(std::string node_name);

};

class ArmCorePublisher : public ArmCore
{

};

class ArmCoreSubscriber : public ArmCore
{

};