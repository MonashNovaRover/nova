#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class manages the KDL model of the arm.
It handles integration of different arm and camera configurations.
It does not compute any forward or inverse kinematics.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: None
TOPICS: None
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	 control
AUTHOR(S):   Jory Braun
CREATION:	 22/01/2022
EDITED:		 23/04/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Compare to KDL examples, see how they structure things.
     Have the tree as a member, or inherit from tree directly?
 - Clean up variable names in all arm submodules.
     Make segment names more descriptive, see if can remove joint names
 - Make frame transformations for J4 hook and for cameras
 - Make frame transformations for end effector grippers and interfaces
 - Check exit_values from addTree and addSegment - add to ArmSubModule for addSegment?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include <vector>
#include <string>

#include "arm_submodule.h"
#include "../arm_configuration.h"

#include <kdl/tree.hpp>


class ArmModel : public KDL::Tree
{
    //------------------------------------------------------------//
    public:

    // List names of all joints. Use for constructing JointState and MultiDOFJointState messages
    std::vector<std::string> joint_names;
    // List names of all endpoints (cameras and tips of end effectors) for constructing messages
    // Also used for IK and endpoint-frame-control
    std::vector<std::string> endpoint_names;
    // Add variable for the default end effcetor (set by the end effector module)
    std::string default_endpoint_name;
    // List names of all segments. Used for calculating FK at all coordinate systems on the arm
    std::vector<std::string> segment_names;

    // List joint limits. Indexed to match joint_names
    std::vector<ArmSubModule::JointLimit> joint_limits;
    
    /// @brief  Constructor. Builds the arm with the given wrist and end effector.
    ///         Builds the arm out of submodules, with separate modules for the lower joints, wrist and end effector.
    ///         Different modules can be swapped out for another of the same type (eg: swap wrists)
    ///         Each module represents a physical assembly that can be attached or detached to/from the arm.
    ///         Each module is based on a KDL::Tree, and defines its own joints, segments, endpoints (cameras and end effectors) and joint limits.
    ///         Each also includes a output 'hook' for attaching the next module to or for defining the arm default endpoint (end effector)
    ArmModel(ArmConfig::WristType wrist_type, ArmConfig::EndEffectorType end_effector_type);
};
