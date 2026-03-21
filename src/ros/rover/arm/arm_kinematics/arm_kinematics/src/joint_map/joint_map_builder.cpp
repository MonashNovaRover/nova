//
// Created by Codex on 21/03/2026.
//

#include "arm_kinematics/joint_map/joint_map_builder.hpp"

#include <stdexcept>

namespace arm_kinematics {

JointMap JointMapBuilder::build(
  const std::vector<std::string> & input_names,
  const std::vector<std::string> & output_names) const
{
  auto built = build_expected(input_names, output_names, JointQuantity::Position);
  if (!built)
    throw std::runtime_error(built.error());

  return std::move(built.value());
}

} // namespace arm_kinematics
