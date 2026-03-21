//
// Created by Codex on 21/03/2026.
//

#ifndef ARM_KINEMATICS_AFFINE_JOINT_MAP_HPP
#define ARM_KINEMATICS_AFFINE_JOINT_MAP_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <urdf/model.h>

#include "arm_kinematics/utilities/span.hpp"
#include "arm_kinematics/visibility_control.h"

namespace arm_kinematics {

class ARM_KINEMATICS_PUBLIC AffineJointMap {
public:
  AffineJointMap() = default;
  AffineJointMap(
    const std::vector<std::string> & input_names,
    const std::vector<std::string> & output_names,
    const std::map<std::string, std::shared_ptr<urdf::JointMimic>> & mimic_joints = {});

  static AffineJointMap identity(size_t element_count);

  void map(span<const double> inputs, span<float> outputs) const;
  void map(const std::vector<double> & inputs, std::vector<float> & outputs) const {
    map(
      {inputs.data(), inputs.size()},
      {outputs.data(), outputs.size()});
  }

  [[nodiscard]] size_t input_count() const noexcept { return input_count_; }
  [[nodiscard]] size_t output_count() const noexcept { return output_count_; }

private:
  static size_t find_source(
    const std::vector<std::string> & joint_names,
    std::map<std::string, std::shared_ptr<urdf::JointMimic>> mimic_joints,
    const std::string & name,
    float & multiplier,
    float & offset);

  std::vector<size_t> sources_{};
  std::vector<float> multipliers_{};
  std::vector<float> offsets_{};
  size_t input_count_ = 0;
  size_t output_count_ = 0;
};

} // arm_kinematics

#endif //ARM_KINEMATICS_AFFINE_JOINT_MAP_HPP
