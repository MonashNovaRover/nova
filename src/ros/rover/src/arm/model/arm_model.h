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
 - Clean up variable names in all arm submodules.
     Make segment names more descriptive, see if can remove joint names
 - Update comments linking to Arm/DH parameters on Grabcad. Link to Nuclino instead?
 - Make frame transformations for J4 hook and for cameras
 - Check exit_values from addTree and addSegment - add to ArmSubModule for addSegment?
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
    // List names of all endpoints (cameras and tips of end effectors) for constructing messages
    // Also used for IK and endpoint-frame-control
    std::vector<std::string> endpoint_names;
    // Add variable for the default end effcetor (set by the end effector module)
    std::string default_endpoint_name;
    // List names of all segments. Used for calculating FK at all coordinate systems on the arm
    std::vector<std::string> segment_names;
    
    /// Constructor. Builds the arm with the given wrist and end effector
    ArmModel(WristType wrist_type = WRIST_CYCLOIDAL, EndEffectorType end_effector_type = EE_EQUIPMENT_SERVICING);
};
