//
// Created by Codex on 21/03/2026.
//

#ifndef ARM_KINEMATICS_TRANSMISSION_ANALYSIS_HPP
#define ARM_KINEMATICS_TRANSMISSION_ANALYSIS_HPP

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "arm_kinematics/joint_map/transmission_model.hpp"
#include "arm_kinematics/joint_map/transmission_types.hpp"
#include "arm_kinematics/visibility_control.h"

namespace arm_kinematics {

class ARM_KINEMATICS_PUBLIC TransmissionAnalysis {
public:
  struct Group {
    ModelId model_id = 0;
    std::vector<JointId> input_joint_ids;
    std::vector<JointId> output_joint_ids;
    bool supports_forward = false;
    bool supports_reverse = false;
  };

  TransmissionAnalysis() = default;
  TransmissionAnalysis(TransmissionAnalysis &&) noexcept = default;
  TransmissionAnalysis & operator=(TransmissionAnalysis &&) noexcept = default;
  TransmissionAnalysis(const TransmissionAnalysis &) = delete;
  TransmissionAnalysis & operator=(const TransmissionAnalysis &) = delete;
  ~TransmissionAnalysis() = default;

  ModelId add_model(std::unique_ptr<TransmissionModel> model);

  [[nodiscard]] bool empty() const noexcept { return groups_.empty(); }
  [[nodiscard]] const std::vector<std::unique_ptr<TransmissionModel>> & models() const noexcept { return models_; }
  [[nodiscard]] const std::vector<Group> & groups() const noexcept { return groups_; }
  [[nodiscard]] const std::vector<std::string> & joint_names() const noexcept { return joint_names_; }
  [[nodiscard]] bool contains_joint(const std::string & name) const noexcept;
  [[nodiscard]] std::optional<JointId> find_joint_id(const std::string & name) const noexcept;

private:
  JointId ensure_joint_id(const std::string & name);

  std::vector<std::unique_ptr<TransmissionModel>> models_{};
  std::vector<Group> groups_{};
  std::vector<std::string> joint_names_{};
  std::map<std::string, JointId> joint_ids_{};
};

} // namespace arm_kinematics

#endif // ARM_KINEMATICS_TRANSMISSION_ANALYSIS_HPP
