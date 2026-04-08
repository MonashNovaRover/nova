//
// Created by Bailey Chessum on 22/03/2026.
//

#ifndef ARM_KINEMATICS_COMPOSITE_JOINT_MAP_HPP
#define ARM_KINEMATICS_COMPOSITE_JOINT_MAP_HPP

#include <vector>

#include "arm_kinematics/joint_map/joint_map.hpp"
#include "arm_kinematics/visibility_control.h"

namespace arm_kinematics {

struct CompositeJointMapSegment {
  JointMap joint_map{};
  std::vector<size_t> output_indices{};
};

class ARM_KINEMATICS_PUBLIC CompositeJointMap {
public:
  CompositeJointMap() = default;
  explicit CompositeJointMap(std::vector<CompositeJointMapSegment> segments);

  void map(span<const float> inputs, span<float> outputs) const;

  [[nodiscard]] size_t input_count() const noexcept { return input_count_; }
  [[nodiscard]] size_t output_count() const noexcept { return output_count_; }

private:
  struct Workspace {
    std::vector<std::vector<float>> segment_outputs{};
  };

  [[nodiscard]] Workspace make_workspace() const;

  std::vector<CompositeJointMapSegment> segments_{};
  size_t input_count_ = 0;
  size_t output_count_ = 0;
  mutable Workspace workspace_{};
};

class ARM_KINEMATICS_PUBLIC StagedJointMap {
public:
  StagedJointMap() = default;
  explicit StagedJointMap(std::vector<JointMap> stages);

  void map(span<const float> inputs, span<float> outputs) const;

  [[nodiscard]] size_t input_count() const noexcept { return input_count_; }
  [[nodiscard]] size_t output_count() const noexcept { return output_count_; }

private:
  struct Workspace {
    std::vector<std::vector<float>> stage_inputs{};
    std::vector<std::vector<float>> stage_outputs{};
  };

  [[nodiscard]] Workspace make_workspace() const;

  std::vector<JointMap> stages_{};
  size_t input_count_ = 0;
  size_t output_count_ = 0;
  mutable Workspace workspace_{};
};

// NOTE: The legacy `compile_joint_map_plan_expected` helper has been removed in step 2 of the
// state-interface refactor (along with the JointMapPlan type it consumed). The new direct
// construction path (driven by JointMapBlueprint segments) will be added in step 5 / step 6.

} // namespace arm_kinematics

#endif // ARM_KINEMATICS_COMPOSITE_JOINT_MAP_HPP
