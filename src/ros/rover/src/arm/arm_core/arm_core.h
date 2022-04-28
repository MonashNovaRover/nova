#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class implements helper functions for other nodes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: None
TOPICS: None
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	 control
AUTHOR(S): Jory Braun
CREATION:	 17/01/2022
EDITED:		 01/02/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include message types
#include "sensor_msgs/msg/joint_state.hpp"
#include "sensor_msgs/msg/multi_dof_joint_state.hpp"

// Include other libraries
#include <vector>
#include <string>


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
