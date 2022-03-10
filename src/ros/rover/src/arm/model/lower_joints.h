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
    constexpr static double BASE_ROTATION_OFFSET = 0.128;
    constexpr static double SHOULDER_OFFSET = 0.126;
    constexpr static double SHOULDER_ELBOW_LINK_LENGTH = 0.485;
    constexpr static double ELBOW_OFFSET = -0.1006;
    constexpr static double ELBOW_OUTPUT_OFFSET = 0.05103;

    /// Constructor. Build the lower joints
    LowerJointsModel()
    {
        // Initialise public members
        joint_names = std::vector<std::string> {"base-rotation", "shoulder", "elbow"};
        // No control points
        output_name = "sj3r";
        zero_angles = std::vector<double> {0, M_PI / 2, 0};
        joint_limits = std::vector<JointLimit> {
            {-2 * M_PI, 2 * M_PI},  // No joint limiting
            {-1.65, 1.65},
            {-2.70, 0.50}  // Fix the upper limit. Not correct
        };
        
        // Build the lower joints
        // Base rotation
        KDL::Joint j1 = KDL::Joint(joint_names[0], KDL::Joint::RotZ);
        KDL::Frame fj1 = KDL::Frame::DH(0, M_PI / 2, BASE_ROTATION_OFFSET, zero_angles[0]);
        this->addSegment(KDL::Segment("sj1", j1, fj1), "root");

        // Shoulder
        KDL::Joint j2 = KDL::Joint(joint_names[1], KDL::Joint::RotZ);
        KDL::Frame fj2 = KDL::Frame::DH(0, 0, SHOULDER_OFFSET, zero_angles[1]);
        this->addSegment(KDL::Segment("sj2", j2, fj2), "sj1");

        // Rigid link from shoulder to elbow
        KDL::Joint j2r = KDL::Joint("rigid-shoulder-to-elbow", KDL::Joint::None);
        KDL::Frame fj2r = KDL::Frame::DH(SHOULDER_ELBOW_LINK_LENGTH, 0, 0, 0);
        this->addSegment(KDL::Segment("sj2r", j2r, fj2r), "sj2");

        // Elbow
        KDL::Joint j3 = KDL::Joint(joint_names[2], KDL::Joint::RotZ);
        KDL::Frame fj3 = KDL::Frame::DH(0, -M_PI / 2, ELBOW_OFFSET, zero_angles[2]);
        this->addSegment(KDL::Segment("sj3", j3, fj3), "sj2r");

        // Rigid link from elbow to output interface
        KDL::Joint j3r = KDL::Joint("rigid-elbow-to-output", KDL::Joint::None);
        KDL::Frame fj3r = KDL::Frame::DH(0, 0, ELBOW_OUTPUT_OFFSET, 0);
        this->addSegment(KDL::Segment("sj3r", j3r, fj3r), "sj3");
    }

};