#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This file defines an abstract class for modelling submodules
  of the arm using KDL.
Each submodule is a discrete mechanical unit that can be
  attached / detached from the arm. For example, the 
  cycloidal wrist or the ES end effector.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: None
TOPICS: None
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	 control
AUTHOR(S):   Jory Braun
CREATION:	 30/01/2022
EDITED:		 30/01/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Item One
 - Item Two
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>
#include <string>

#include <kdl/tree.hpp>


class ArmSubModule : public KDL::Tree
{
    //------------------------------------------------------------//
    public:

    /// Default constructor. Not used here, only relevant for inherited classes
    ArmSubModule()
    {
        // Typical structure of a module constructor:

        // Initialise public members:
        //   1. joint_names
        //   2. control_point_names
        //   3. output_name
        //   4. zero_angles
        
        // Build the module segment by segment
        // For each segment, will consist of:
        //   1. A joint. For DH frames, will have type KDL::Joint::RotZ. For rigid connections, will have KDL::Joint::None
        //   2. A frame transformation. Is a transformation from the current frame to the parent frame.
        //        Equivalently, it defines the coordinate axes and origin position of the current frame within the 
        //        coord system of the parent frame.
        //        Can be defined using DH / modified-DH parameters, or through general coordinate transformations
        //   3. A segment. Constructed from the joint and frame, and then added to the arm at the tip of an existing
        //        segment, or "root" if there is no segment or is attached to the base of the first segment.
    };

    // List names of all joints use for constructing JointState and MultiDOFJointState messages
    std::vector<std::string> joint_names;
    // List names of all control points (cameras and tips of end effectors) for constructing messages
    // Also used for IK and camera-frame-control
    std::vector<std::string> control_point_names;
    // Name of the segment where any subsequent modules can attach
    std::string output_name;

    // List model angles of each joint when the arm is in the resolver zeroing position.
    // The model will be initialised in this position, and all angles measured relative to it.
    std::vector<double> zero_angles;

};