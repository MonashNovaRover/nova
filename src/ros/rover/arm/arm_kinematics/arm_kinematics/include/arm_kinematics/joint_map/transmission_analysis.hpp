//
// Created by Bailey Chessum on 21/03/2026.
//

#ifndef ARM_KINEMATICS_TRANSMISSION_ANALYSIS_HPP
#define ARM_KINEMATICS_TRANSMISSION_ANALYSIS_HPP

#include <memory>
#include <string>
#include <vector>

#include "arm_kinematics/joint_map/transmission_model.hpp"
#include "arm_kinematics/joint_map/transmission_types.hpp"
#include "arm_kinematics/utilities/order.hpp"
#include "arm_kinematics/visibility_control.h"

namespace arm_kinematics {

class ARM_KINEMATICS_PUBLIC TransmissionAnalysis {
private:
  struct TransmissionInstance {
    TransmissionModelId model_id = 0;
    std::vector<JointId> input_joint_ids;
    std::vector<JointId> output_joint_ids;

    std::string name;   //< only used for logging to give info about invalid configurations!
    // forward and backward support determined by the TransmissionModel
  };

public:
  TransmissionAnalysis() = default;
  TransmissionAnalysis(TransmissionAnalysis &&) noexcept = default;
  TransmissionAnalysis & operator=(TransmissionAnalysis &&) noexcept = default;
  TransmissionAnalysis(const TransmissionAnalysis &) = delete;
  TransmissionAnalysis & operator=(const TransmissionAnalysis &) = delete;
  ~TransmissionAnalysis() = default;

  [[nodiscard]] const std::vector<std::unique_ptr<TransmissionModel>> & models() const noexcept { return models_; }
  TransmissionModelId add_model(std::unique_ptr<TransmissionModel> model);

  // [[nodiscard]] bool empty() const noexcept { return transmissions_.empty(); } //< Use transmissions().empty() -- more explicit as to what is actually empty
  [[nodiscard]] const std::vector<TransmissionInstance> & transmissions() const noexcept { return transmissions_; }

  /// Canonical boundary mapping from named joints in descriptions to stable internal JointIds.
  [[nodiscard]] const Order<std::string, JointId> & joint_order() const noexcept { return joint_order_; }
  // [[nodiscard]] bool contains_joint_id(JointId joint_id) const noexcept; //< Just use joint_order().inverse.contains_key()
  /// provides the JointID from joint_order_, adding it to the end of the order if it is not already present.
  JointId ensure_joint_id(const std::string & name);

  void add_transmission(
    TransmissionModelId model_id,
    std::vector<JointId> && inputs,
    std::vector<JointId> && outputs,
    std::string name = "unnamed");
  /// Convenience overload, which calls the above overload that uses JointID
  void add_transmission(
    TransmissionModelId model_id,
    span<const std::string> inputs,
    span<const std::string> outputs,
    std::string name = "unnamed");

private:
  std::vector<std::unique_ptr<TransmissionModel>> models_{};
  std::vector<TransmissionInstance> transmissions_{};
  Order<std::string, JointId> joint_order_{};
};

} // namespace arm_kinematics

#endif // ARM_KINEMATICS_TRANSMISSION_ANALYSIS_HPP
