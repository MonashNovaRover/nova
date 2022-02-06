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

// Include message types
#include "sensor_msgs/msg/joint_state.hpp"
#include "sensor_msgs/msg/multi_dof_joint_state.hpp"

// Include other libraries
#include <vector>
#include <string>

// Arm configuration related defines
// Eventually remove and use dynamic joint_names, model_joint_names, etc
#define NUM_JOINTS 6


class ArmCore
{  
    //------------------------------------------------------------//
    public:

    /// @brief  Helper function to construct empty JointState message
    ///         Uses given names of joints, sizes all other parameter lists to match
    static sensor_msgs::msg::JointState get_empty_joint_state(const std::vector<std::string>& names);

    /// @brief  Helper function to construct empty MultiDOFJointState message
    ///         Uses given names of joints, sizes all other parameter lists to match
    static sensor_msgs::msg::MultiDOFJointState get_empty_multi_dof_joint_state(const std::vector<std::string>& names);

    //------------------------------------------------------------//
    protected:
    
    /// @brief  Protected constructor so the class cannot be instantiated
    ArmCore(){}

};
