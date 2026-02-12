//
// Created by Bailey Chessum on 15/10/2025.
//

#include "arm_kinematics/common/kinematics_params.hpp"
#include "arm_kinematics/utilities/param_reader.hpp"

namespace arm_kinematics {

KinematicsParams::KinematicsParams(const NodeParametersInterface::SharedPtr & node)
{
  const ParamReader params(node);

  base_link_name = params.get<std::string>("base_link_name", "base_link");
  ee_link_name = params.get<std::string>("ee_link_name", "ee_link");

  joint_names = params.get<std::vector<std::string>>(
    "joint_names",
    {},
    "The names of all joints in the default kinematics chain, from the base link to the end effector");
}

KinematicsParams::KinematicsParams(NodeInterfaces node)
: KinematicsParams(node.get_node_parameters_interface())
{
}

} // arm_kinematics