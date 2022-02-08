#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class defines the KDL model of the cycloidal wrist
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


class WristCycloidalModel : public ArmSubModule
{
    //------------------------------------------------------------//
    public:

    // Parameters for arm model geometry. Based on model in Arm/DH parameters on GrabCAD
    // All distances in mm, all angles in rad
    constexpr static double J4_LINK_LENGTH = 499;  // Distance from z3 to z4 (not parallel to actual link)
    constexpr static double J5_OFFSET = 104;  // Distance from p4 to p56 along z5
    
    constexpr static double HOOK_OFFSET_X = -99;  // Distances from p4 to pE2 along axes xyzE2
    constexpr static double HOOK_OFFSET_Y = -24;
    constexpr static double HOOK_OFFSET_Z = 75;
    constexpr static double HOOK_ANGLE_X = -5.87 * M_PI/180;  // Angle from x3 to the axis of the cylindrical link


    /// Constructor. Build the cycloidal wrist
    WristCycloidalModel()
    {
        // Initialise public members
        joint_names = std::vector<std::string> {"j4", "j5", "j6"};
        control_point_names = std::vector<std::string> {"j4-hook", "squooshy"};
        output_name = "sj6";
        zero_angles = std::vector<double> {0, -M_PI / 2, 0};

        // Build the cycloidal wrist
        // J4
        KDL::Joint j4 = KDL::Joint(joint_names[0], KDL::Joint::RotZ);
        KDL::Frame fj4 = KDL::Frame::DH_Craig1989(J4_LINK_LENGTH, 0, 0, zero_angles[0]);
        this->addSegment(KDL::Segment("sj4", j4, fj4), "root");

        // J5
        KDL::Joint j5 = KDL::Joint(joint_names[1], KDL::Joint::RotZ);
        KDL::Frame fj5 = KDL::Frame::DH_Craig1989(0, M_PI / -2, J5_OFFSET, zero_angles[1]);
        this->addSegment(KDL::Segment("sj5", j5, fj5), "sj4");

        // J6
        KDL::Joint j6 = KDL::Joint(joint_names[2], KDL::Joint::RotZ);
        KDL::Frame fj6 = KDL::Frame::DH_Craig1989(0, M_PI / -2, 0, zero_angles[2]);
        this->addSegment(KDL::Segment("sj6", j6, fj6), "sj5");
        
        // j4-hook
        KDL::Joint hook = KDL::Joint(control_point_names[0], KDL::Joint::None);
        // Construct the frame. Make this more efficient.
        KDL::Frame hook_to_j4 = KDL::Frame(KDL::Vector(HOOK_OFFSET_X, HOOK_OFFSET_Y, HOOK_OFFSET_Z));
        KDL::Rotation j4_to_elbow_rot = KDL::Rotation::Identity();
        j4_to_elbow_rot.DoRotZ(M_PI);
        j4_to_elbow_rot.DoRotY(-M_PI / 2);
        j4_to_elbow_rot.DoRotX(HOOK_ANGLE_X);
        // Check this rotation matrix.
        // Check j4_to_elbow_rot, make sure rotation part matches with transformation_j4_to_elbow in old model.py 
        KDL::Frame j4_to_elbow = KDL::Frame(j4_to_elbow_rot, KDL::Vector(J4_LINK_LENGTH, 0, 0));
        KDL::Frame fhook = j4_to_elbow * hook_to_j4;
        // Create segment, add to the tree
        this->addSegment(KDL::Segment("shook", hook, fhook), "root");

        // Squooshy
        KDL::Joint squooshy = KDL::Joint(control_point_names[1], KDL::Joint::None);
        // Put at the root for now, can move it to the correct spot later
        KDL::Frame fsquooshy = KDL::Frame::Identity();
        this->addSegment(KDL::Segment("ssquooshy", squooshy, fsquooshy), "root");
    }

};