// Created by Bailey Chessum on 12/04/2026.
//
// CURRENTLY UNUSED!
// But could be a useful helper if we ever need to extend what joints a TransmissionAnalysis has without mutating the
// original TranmsissionAnalysis (for multithreaded contexts). However, the current design will simply enforce that all
// joints are registered in the TransmissionAnalysis for now.

#ifndef ARM_KINEMATICS_TRANSMISSION_ANALYSIS_VIEW_HPP
#define ARM_KINEMATICS_TRANSMISSION_ANALYSIS_VIEW_HPP

#include <string>
#include <unordered_map>
#include <vector>

#include "arm_kinematics/joint_map/affine_projection_rule.hpp"
#include "arm_kinematics/joint_map/state_interface_definition.hpp"
#include "arm_kinematics/joint_map/transmission_analysis.hpp"
#include "arm_kinematics/joint_map/transmission_types.hpp"
#include "arm_kinematics/utilities/interface_id.hpp"
#include "arm_kinematics/utilities/span.hpp"
#include "arm_kinematics/visibility_control.h"

namespace arm_kinematics {

/**
 * Non-owning adapter over a `TransmissionAnalysis` that automatically extends the ID space for
 * joints and state interfaces that are not registered in the underlying analysis.
 *
 * The wrapped analysis is called the "base". Extension joints and state interfaces receive IDs
 * that start immediately after the base's current ID ranges:
 *   - Extension JointIds start at `base().joint_order().size()`.
 *   - Extension StateInterfaceIds start at `base().state_interface_order().size()`.
 *
 * Extension nodes have no transmission relationships — they behave as isolated pass-throughs:
 *   - `affine_transmission_of(j)` returns the identity `AffineTransmission{j, j, 1.0, 0.0}`.
 *   - `affine_root_of(j)` returns `j`.
 *   - `affine_group_members(j)` returns a singleton span containing only `j`.
 *   - `producing_transmissions(sid)` returns an empty span.
 *
 * All other queries — `transmissions()`, `affine_projection_rules()` — delegate to the base.
 *
 * \warning The base `TransmissionAnalysis` must outlive this view and must NOT be mutated after
 * the view is constructed. Base joint/state-interface counts are captured at construction time;
 * appending to the base afterwards causes undefined behaviour.
 */
class ARM_KINEMATICS_PUBLIC TransmissionAnalysisView {
public:
  using AffineTransmission   = TransmissionAnalysis::AffineTransmission;
  using TransmissionInstance = TransmissionAnalysis::TransmissionInstance;
  using StateInterfaceId     = TransmissionAnalysis::StateInterfaceId;

  explicit TransmissionAnalysisView(const TransmissionAnalysis & base);

  // Non-copyable, non-movable — holds a reference to the base.
  TransmissionAnalysisView(const TransmissionAnalysisView &) = delete;
  TransmissionAnalysisView & operator=(const TransmissionAnalysisView &) = delete;
  TransmissionAnalysisView(TransmissionAnalysisView &&) = delete;
  TransmissionAnalysisView & operator=(TransmissionAnalysisView &&) = delete;
  ~TransmissionAnalysisView() = default;

  // ---------------------------------------------------------------------------
  // Base access
  // ---------------------------------------------------------------------------

  [[nodiscard]] const TransmissionAnalysis & base() const noexcept { return base_; }

  // ---------------------------------------------------------------------------
  // ID resolution (mirrors TransmissionAnalysis public API)
  // ---------------------------------------------------------------------------

  /// Returns the JointId for `name`. Delegates to the base if the name is known; otherwise
  /// auto-registers it in the extension registry with the next available extension ID.
  [[nodiscard]] JointId ensure_joint_id(const std::string & name);

  /// Returns the StateInterfaceId for `definition`. Delegates to the base if known; otherwise
  /// auto-registers in the extension registry. The `definition.joint_id` must already be a valid
  /// ID from either the base or this view's extension registry.
  [[nodiscard]] StateInterfaceId ensure_state_interface_id(const StateInterfaceDefinition & definition);

  /// Convenience overload — resolves the joint name via `ensure_joint_id` first.
  [[nodiscard]] StateInterfaceId ensure_state_interface_id(const NamedStateInterfaceDefinition & definition);

  // ---------------------------------------------------------------------------
  // Counts
  // ---------------------------------------------------------------------------

  /// Total number of joints (base + extension).
  [[nodiscard]] std::size_t joint_count() const noexcept;

  /// Total number of state interfaces (base + extension).
  [[nodiscard]] std::size_t state_interface_count() const noexcept;

  // ---------------------------------------------------------------------------
  // Affine group queries (dispatch: base or extension)
  // ---------------------------------------------------------------------------

  [[nodiscard]] const AffineTransmission & affine_transmission_of(JointId j) const noexcept;
  [[nodiscard]] JointId affine_root_of(JointId j) const noexcept;
  [[nodiscard]] span<const JointId> affine_group_members(JointId root) const noexcept;

  // ---------------------------------------------------------------------------
  // Inverse transmission index (dispatch: base or extension)
  // ---------------------------------------------------------------------------

  [[nodiscard]] span<const TransmissionInstanceId> producing_transmissions(StateInterfaceId sid) const noexcept;

  // ---------------------------------------------------------------------------
  // Delegated to base
  // ---------------------------------------------------------------------------

  [[nodiscard]] const std::vector<TransmissionInstance> & transmissions() const noexcept
  {
    return base_.transmissions();
  }

  [[nodiscard]] const Order<InterfaceId, ProjectionKindId> & projection_order() const noexcept
  {
    return base_.projection_order();
  }

  [[nodiscard]] const std::vector<AffineProjectionRule> & affine_projection_rules() const noexcept
  {
    return base_.affine_projection_rules();
  }

  [[nodiscard]] const AffineProjectionRule * affine_projection_rule(const InterfaceId & interface_id) const noexcept
  {
    return base_.affine_projection_rule(interface_id);
  }

private:
  const TransmissionAnalysis & base_;

  std::size_t base_joint_count_;           ///< base_.joint_order().size() at construction.
  std::size_t base_state_interface_count_; ///< base_.state_interface_order().size() at construction.

  /// Extension joint name → JointId. IDs start at `base_joint_count_`.
  std::unordered_map<std::string, JointId> ext_joint_ids_;

  /// Extension state interface definition → StateInterfaceId. IDs start at `base_state_interface_count_`.
  std::unordered_map<StateInterfaceDefinition, StateInterfaceId> ext_state_interface_ids_;

  /// Per-extension-joint identity AffineTransmission. Indexed by (joint_id - base_joint_count_).
  /// Stable storage so `affine_transmission_of()` can return `const &`.
  std::vector<AffineTransmission> ext_affine_transmissions_;

  /// Per-extension-joint singleton member list {j} for `affine_group_members()`.
  /// Indexed by (joint_id - base_joint_count_).
  std::vector<std::vector<JointId>> ext_group_members_;
};

}  // namespace arm_kinematics

#endif  // ARM_KINEMATICS_TRANSMISSION_ANALYSIS_VIEW_HPP
