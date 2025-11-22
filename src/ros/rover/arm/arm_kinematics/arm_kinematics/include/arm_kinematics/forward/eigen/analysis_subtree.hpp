//
// Created by nova on 22/11/25.
//

#ifndef ARM_KINEMATICS_ANALYSIS_SUBTREE_H
#define ARM_KINEMATICS_ANALYSIS_SUBTREE_H
#include "analysis_tree.hpp"

namespace arm_kinematics
{

/**
 * A subtree of an AnalysisTree with a unique root
 */
class AnalysisSubtree
{
private:
  AnalysisSubtree(const AnalysisTree & tree) {}


public:
  AnalysisSubtree(
    const AnalysisTree & tree,
    const std::string & root_name,
    const FrameDefinitions & definitions);

const AnalysisTree & tree;



};

} // arm_kinematics

#endif //ARM_KINEMATICS_ANALYSIS_SUBTREE_H