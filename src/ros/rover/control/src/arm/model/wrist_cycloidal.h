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
EDITED:		 23/04/2022
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

    // Parameters for arm model geometry
    // All distances in mm, all angles in rad
    constexpr static double ROOT_J4_LINK_LENGTH = 0.499;
    constexpr static double J4_OFFSET = 0.09952;
    constexpr static double J5_OFFSET = 0.1041;
    constexpr static double J6_OFFSET = 0.1079;

    // Parameters for the cycloidal wrist CMDs
    constexpr static double GEARBOX_REDUCTION = 3002.499;
    const static int ENCODER_PPR = 256;
    const static int MIN_INTERVAL = 200;
    constexpr static double CLOCK_FREQUENCY = 30e6;

    /// Constructor. Build the cycloidal wrist
    WristCycloidalModel(bool can_init=1)
    {
        // Initialise public members
        module_name = "wrist_cycloidal";
        joint_names = std::vector<std::string> {"j4", "j5", "j6"};
        endpoint_names = std::vector<std::string> {"j4-hook", "squooshy"};
        output_name = "sj6";
        zero_angles = std::vector<double> {0, -M_PI / 2, 0};
        joint_limits = std::vector<JointLimit> {
            {-1.95, 1.57},
            {-1.95, 1.75},
            {-2 * M_PI, 2 * M_PI}  // No joint limiting
        };
        control_coeffs = std::vector<ControlCoeffs> {
            // P, I, D
            {1, 1, 0},
            {1, 1, 0},
            {1, 1, 0}
        };
        double scaling_factor = CMD::get_scaling_factor(GEARBOX_REDUCTION, ENCODER_PPR, MIN_INTERVAL, CLOCK_FREQUENCY);
        drivers = std::vector<CMD*> {
            new CMD(1, 4, PID, 1, STOP, scaling_factor, can_init),
            new CMD(1, 5, PID, 1, STOP, scaling_factor, can_init),
            new CMD(1, 6, PID, 1, STOP, scaling_factor, can_init)
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
