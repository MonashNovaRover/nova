//
// Created by Bailey Chessum on 21/03/2026.
//

#include "arm_kinematics/joint_map/joint_map_builder.hpp"

#include <stdexcept>

namespace arm_kinematics {

JointMap JointMapBuilder::build(
  const std::vector<StateInterfaceDefinition> & input_names,
  const std::vector<StateInterfaceDefinition> & output_names) const
{
  auto built = build_expected(input_names, output_names);
  if (!built)
    throw std::runtime_error(built.error());

  return std::move(built.value());
}

} // namespace arm_kinematics
