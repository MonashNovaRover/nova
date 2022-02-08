/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class extends KDL::TreeSolverPos and adds additional
  inheritance features such as a default constructor and
  assignment operator.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
AUTHOR(S):   Jory Braun
CREATION:	 27/01/2022
EDITED:		 27/01/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#ifndef KDLTREEFKSOLVERPOS_RECURSIVE_EXT_HPP
#define KDLTREEFKSOLVERPOS_RECURSIVE_EXT_HPP

#include <kdl/treefksolverpos_recursive.hpp>

namespace KDL {

    class TreeFkSolverPos_recursive_ext : public TreeFkSolverPos_recursive
    {
    public:
        // Add default constructor
        TreeFkSolverPos_recursive_ext();
        // Recreate constructor from base class
        TreeFkSolverPos_recursive_ext(const Tree& tree);

        // Add assignment operator
        void operator= (const TreeFkSolverPos_recursive_ext& arg);
    
    private:
        // Redefine Tree so it is not const and is accessible in this class
        Tree tree;

    };
}

#endif