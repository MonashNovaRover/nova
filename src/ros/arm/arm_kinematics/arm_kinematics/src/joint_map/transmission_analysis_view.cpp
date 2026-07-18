// Created by Bailey Chessum on 12/04/2026.
//
// CURRENTLY UNUSED!
// But could be a useful helper if we ever need to extend what joints a TransmissionAnalysis has without mutating the
// original TranmsissionAnalysis (for multithreaded contexts). However, the current design will simply enforce that all
// joints are registered in the TransmissionAnalysis for now.

#include "arm_kinematics/joint_map/transmission_analysis_view.hpp"

namespace arm_kinematics {

using StateInterfaceId = TransmissionAnalysisView::StateInterfaceId;

TransmissionAnalysisView::TransmissionAnalysisView(const TransmissionAnalysis & base)
  : base_(base),
    base_joint_count_(base.joint_order().size()),
    base_state_interface_count_(base.state_interface_order().size())
{
}

JointId TransmissionAnalysisView::ensure_joint_id(const std::string & name)
{
  if (base_.joint_order().contains_key(name)) {
    return base_.joint_order()[name];
  }

  const auto it = ext_joint_ids_.find(name);
  if (it != ext_joint_ids_.end()) {
    return it->second;
  }

  const JointId new_id = base_joint_count_ + static_cast<JointId>(ext_joint_ids_.size());
  ext_joint_ids_.emplace(name, new_id);
  ext_affine_transmissions_.push_back(AffineTransmission{new_id, new_id, 1.0, 0.0});
  ext_group_members_.push_back({new_id});
  return new_id;
}

StateInterfaceId TransmissionAnalysisView::ensure_state_interface_id(
  const StateInterfaceDefinition & definition)
{
  if (base_.state_interface_order().contains_key(definition)) {
    return base_.state_interface_order()[definition];
  }

  const auto it = ext_state_interface_ids_.find(definition);
  if (it != ext_state_interface_ids_.end()) {
    return it->second;
  }

  const StateInterfaceId new_id =
    base_state_interface_count_ + static_cast<StateInterfaceId>(ext_state_interface_ids_.size());
  ext_state_interface_ids_.emplace(definition, new_id);
  return new_id;
}

StateInterfaceId TransmissionAnalysisView::ensure_state_interface_id(
  const NamedStateInterfaceDefinition & definition)
{
  return ensure_state_interface_id(StateInterfaceDefinition{
    ensure_joint_id(definition.joint_name),
    definition.interface_id
  });
}

std::size_t TransmissionAnalysisView::joint_count() const noexcept
{
  return base_joint_count_ + ext_joint_ids_.size();
}

std::size_t TransmissionAnalysisView::state_interface_count() const noexcept
{
  return base_state_interface_count_ + ext_state_interface_ids_.size();
}

const TransmissionAnalysisView::AffineTransmission &
TransmissionAnalysisView::affine_transmission_of(const JointId j) const noexcept
{
  if (j < base_joint_count_) {
    return base_.affine_transmission_of(j);
  }
  return ext_affine_transmissions_[j - base_joint_count_];
}

JointId TransmissionAnalysisView::affine_root_of(const JointId j) const noexcept
{
  if (j < base_joint_count_) {
    return base_.affine_root_of(j);
  }
  return j;
}

span<const JointId> TransmissionAnalysisView::affine_group_members(const JointId root) const noexcept
{
  if (root < base_joint_count_) {
    return base_.affine_group_members(root);
  }
  const auto & members = ext_group_members_[root - base_joint_count_];
  return {members.data(), members.size()};
}

span<const TransmissionInstanceId>
TransmissionAnalysisView::producing_transmissions(const StateInterfaceId sid) const noexcept
{
  if (sid < base_state_interface_count_) {
    return base_.producing_transmissions(sid);
  }
  return {};
}

}  // namespace arm_kinematics
