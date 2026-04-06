//
// Created by Bailey Chessum on 5/4/26.
//

#ifndef ARM_KINEMATICS_TRANSMISSIONSUBGRAPH_HPP
#define ARM_KINEMATICS_TRANSMISSIONSUBGRAPH_HPP

#include "transmission_analysis.hpp"

namespace arm_kinematics {

class TransmissionSubgraph
{
public:
  explicit TransmissionSubgraph(
    TransmissionAnalysis & analysis,
    const std::vector<StateInterfaceDefinition> & inputs,
    const std::vector<StateInterfaceDefinition> & outputs)
    : analysis_(analysis)
  {

    // Storage for each transmission

  }

  void add_input(JointId input)
  {
    // Mark all incoming transmissions as deleted


  }



private:
  TransmissionAnalysis & analysis_;
};


} // arm_kinematics

#endif //ARM_KINEMATICS_TRANSMISSIONSUBGRAPH_HPP