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
    constexpr static double ROOT_J4_LINK_LENGTH = 0.499;
    constexpr static double J4_OFFSET = 0.09952;
    constexpr static double J5_OFFSET = 0.1041;
    constexpr static double J6_OFFSET = 0.1079;
    
    /// Constructor. Build the cycloidal wrist
    WristCycloidalModel()
    {
        // Initialise public members
        joint_names = std::vector<std::string> {"j4", "j5", "j6"};
        endpoint_names = std::vector<std::string> {"j4-hook", "squooshy"};
        output_name = "sj6";
        zero_angles = std::vector<double> {0, -M_PI / 2, 0};
        joint_limits = std::vector<JointLimit> {
            {-2.75, 1.10},
            {-1.80, 2.45},
            {-2 * M_PI, 2 * M_PI}  // No joint limiting
        };

        // Build the cycloidal wrist
        // Rigid link from root to j4
        KDL::Joint j3r2 = KDL::Joint("rigid-root-to-j4", KDL::Joint::None);
        KDL::Frame fj3r2 = KDL::Frame::DH(ROOT_J4_LINK_LENGTH, M_PI / 2, 0, 0);
        this->addSegment(KDL::Segment("sj3r2", j3r2, fj3r2), "root");
        
        // J4
        KDL::Joint j4 = KDL::Joint(joint_names[0], KDL::Joint::RotZ);
        KDL::Frame fj4 = KDL::Frame::DH(0, -M_PI / 2, J4_OFFSET, zero_angles[0]);
        this->addSegment(KDL::Segment("sj4", j4, fj4), "sj3r2");

        // J5
        KDL::Joint j5 = KDL::Joint(joint_names[1], KDL::Joint::RotZ);
        KDL::Frame fj5 = KDL::Frame::DH(0, -M_PI / 2, J5_OFFSET, zero_angles[1]);
        this->addSegment(KDL::Segment("sj5", j5, fj5), "sj4");

        // J6
        KDL::Joint j6 = KDL::Joint(joint_names[2], KDL::Joint::RotZ);
        KDL::Frame fj6 = KDL::Frame::DH(0, 0, J6_OFFSET, zero_angles[2]);
        this->addSegment(KDL::Segment("sj6", j6, fj6), "sj5");
        
        // j4-hook
        KDL::Joint hook = KDL::Joint(endpoint_names[0], KDL::Joint::None);
        // Put at the root for now, can move it to the correct spot later
        KDL::Frame fhook = KDL::Frame::Identity();
        this->addSegment(KDL::Segment("shook", hook, fhook), "root");

        // Squooshy
        KDL::Joint squooshy = KDL::Joint(endpoint_names[1], KDL::Joint::None);
        // Put at the root for now, can move it to the correct spot later
        KDL::Frame fsquooshy = KDL::Frame::Identity();
        this->addSegment(KDL::Segment("ssquooshy", squooshy, fsquooshy), "root");
    }

};
