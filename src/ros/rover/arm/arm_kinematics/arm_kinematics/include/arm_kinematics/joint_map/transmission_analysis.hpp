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
public:
  struct AffineTransmission {
    // The joint written to by this affine transmission
    JointId target_joint_id = 0;

    // The joint read by this affine transmission
    JointId source_joint_id = 0;
    float multiplier = 1.0F;
    float offset = 0.0F;
  };

  struct TransmissionInstance {
    TransmissionModelId model_id = 0;
    std::vector<JointId> input_joint_ids;
    std::vector<JointId> output_joint_ids;

    std::string name;   //< only used for logging to give info about invalid configurations!
    // forward and backward support determined by the TransmissionModel
  };

  TransmissionAnalysis() = default;
  TransmissionAnalysis(const TransmissionAnalysis & other);
  TransmissionAnalysis(TransmissionAnalysis &&) noexcept = default;
  TransmissionAnalysis & operator=(const TransmissionAnalysis & other);
  TransmissionAnalysis & operator=(TransmissionAnalysis &&) noexcept = default;
  ~TransmissionAnalysis() = default;

  [[nodiscard]] const std::vector<std::unique_ptr<TransmissionModel>> & models() const noexcept { return models_; }
  TransmissionModelId add_model(std::unique_ptr<TransmissionModel> model);

  [[nodiscard]] const std::vector<TransmissionInstance> & transmissions() const noexcept { return transmissions_; }
  /// a.k.a. mimic joints
  [[nodiscard]] const std::vector<AffineTransmission> & affine_transmissions() const noexcept
  {
    return affine_transmissions_;
  }

  /// Canonical boundary mapping from named joints in descriptions to stable internal JointIds.
  [[nodiscard]] const Order<std::string, JointId> & joint_order() const noexcept { return joint_order_; }
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

  /**
   * Adds one affine transmission relationship to the cached analysis.
   *
   * \warning This API does not validate cycles between affine transmissions. Callers must not add cyclic affine
   * transmission relationships.
   */
  void add_affine_transmission(
    JointId source_joint_id,
    JointId target_joint_id,
    float multiplier = 1.0f,
    float offset = 0.0f);
  /**
   * Adds one affine transmission relationship to the cached analysis.
   *
   * \warning This API does not validate cycles between affine transmissions. Callers must not add cyclic affine
   * transmission relationships.
   */
  void add_affine_transmission(
    const std::string & source_joint_name,
    const std::string & target_joint_name,
    float multiplier = 1.0f,
    float offset = 0.0f);

private:
  std::vector<std::unique_ptr<TransmissionModel>> models_{};
  std::vector<AffineTransmission> affine_transmissions_{};
  std::vector<TransmissionInstance> transmissions_{};
  Order<std::string, JointId> joint_order_{};
};

} // namespace arm_kinematics

#endif // ARM_KINEMATICS_TRANSMISSION_ANALYSIS_HPP
