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
EDITED:		 29/01/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Compare to KDL examples, see how they structure things.
     Have the tree as a member, or inherit from tree directly?
 - Item Two
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include <vector>
#include <string>

#include <kdl/tree.hpp>


class ArmModel : public KDL::Tree
{
    //------------------------------------------------------------//
    public:

    // Define all wrist types
    // These represent KDL::Tree objects that attach at the output of the elbow joint
    // The SPM wrist is replaced by an equivalent spherical serial manipulator (SSM) which includes End Rotation
    // Each wrist defines its own control points representing end effector or camera poses and a hook location for an end effcetor
    typedef enum {
        WRIST_CYCLOIDAL,
        WRIST_SPM
    } WristType;

    // Define all end effector types
    // These represent KDL::Tree objects that attach at the hook location provided by the wrist
    // Each end effector defines its own control points and any additional degrees of freedom
    typedef enum {
        EE_EQUIPMENT_SERVICING,
        EE_EXTREME_RETRIEVAL,
        EE_LUNAR_CONSTRUCTION
    } EndEffectorType;

    // List names of all joints use for constructing JointState and MultiDOFJointState messages
    std::vector<std::string> joint_names;
    // List names of all control points (cameras and tips of end effectors) for constructing messages
    // Also used for IK and camera-frame-control
    std::vector<std::string> control_point_names;

    /// Constructor. Builds the arm with the given wrist and end effector
    ArmModel(WristType wrist_type = 0, EndEffectorType end_effector_type = 0);
};
