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
EDITED:		 22/01/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Compare to KDL examples, see how they structure things.
     Have the tree as a member, or inherit from tree directly?
 - Item Two
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include libraries
#include <kdl/tree.hpp>


class ArmModel
{
    //------------------------------------------------------------//
    private:

    // Description of the arm structure using KDL
    KDL::Tree tree;

    //------------------------------------------------------------//
    public:

    /// Default constructor. Builds the arm.
    ArmModel();

    /// Get the tree that represents the structure of the arm
    const KDL::Tree& get_tree() const { return tree; }
};
