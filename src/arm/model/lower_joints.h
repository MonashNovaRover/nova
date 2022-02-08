#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class defines the KDL model of the lower joints.
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

#include "arm_submodule.h"


class LowerJointsModel : public ArmSubModule
{
    //------------------------------------------------------------//
    public:

    // Parameters for arm model geometry. Based on model in Arm/DH parameters on GrabCAD
    // All distances in mm, all angles in rad
    constexpr static double SHOULDER_OFFSET = 124;  // Distance from p01 to p4 along z2
    constexpr static double ELBOW_LINK_LENGTH = 485;  // Distance from z2 to z3
    // Add intermediary frame for vertical offset at elbow output. Would make angles easier later on

    /// Constructor. Build the lower joints
    LowerJointsModel()
    {
        // Initialise public members
        joint_names = std::vector<std::string> {"base-rotation", "shoulder", "elbow"};
        // No control points
        output_name = "sj3";
        zero_angles = std::vector<double> {0, M_PI / 2, -M_PI / 2};
        
        // Build the lower joints
        // Base rotation
        KDL::Joint j1 = KDL::Joint(joint_names[0], KDL::Joint::RotZ);
        KDL::Frame fj1 = KDL::Frame::DH_Craig1989(0, 0, 0, zero_angles[0]);
        this->addSegment(KDL::Segment("sj1", j1, fj1), "root");

        // Shoulder
        KDL::Joint j2 = KDL::Joint(joint_names[1], KDL::Joint::RotZ);
        KDL::Frame fj2 = KDL::Frame::DH_Craig1989(0, M_PI / 2, SHOULDER_OFFSET, zero_angles[1]);
        this->addSegment(KDL::Segment("sj2", j2, fj2), "sj1");

        // Elbow
        KDL::Joint j3 = KDL::Joint(joint_names[2], KDL::Joint::RotZ);
        KDL::Frame fj3 = KDL::Frame::DH_Craig1989(ELBOW_LINK_LENGTH, 0, 0, zero_angles[2]);
        this->addSegment(KDL::Segment("sj3", j3, fj3), "sj2");
    }

};