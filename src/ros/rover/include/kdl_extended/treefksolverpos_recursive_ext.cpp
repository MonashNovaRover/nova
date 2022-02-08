/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

AUTHOR(S):	Jory Braun
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "treefksolverpos_recursive_ext.hpp"

namespace KDL {

    TreeFkSolverPos_recursive_ext::TreeFkSolverPos_recursive_ext() :
        TreeFkSolverPos_recursive(Tree())
    {
    }

    TreeFkSolverPos_recursive_ext::TreeFkSolverPos_recursive_ext(const Tree& tree) :
        TreeFkSolverPos_recursive(tree)
    {
    }

    void TreeFkSolverPos_recursive_ext::operator= (const TreeFkSolverPos_recursive_ext& in)
    {
        tree = in.tree;
    }

}