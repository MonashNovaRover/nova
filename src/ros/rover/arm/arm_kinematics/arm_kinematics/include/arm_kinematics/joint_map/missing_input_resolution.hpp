//
// Created by Bailey Chessum on 8/4/26.
//

#ifndef ARM_KINEMATICS_MISSING_INPUT_RESOLUTION_HPP
#define ARM_KINEMATICS_MISSING_INPUT_RESOLUTION_HPP

#include <optional>
#include <vector>

#include "arm_kinematics/joint_map/transmission_types.hpp"

namespace arm_kinematics {

class TransmissionSubgraph;  // forward declaration; full def in transmission_subgraph.hpp

/**
 * One way to resolve a single missing (unreachable) state interface in a `TransmissionSubgraph`'s
 * plan. The builder packages a vector of these into `JointMapBuildError::resolutions` so the user
 * sees actionable suggestions for unblocking the build.
 *
 * \note Constructed by `compute_missing_input_resolutions(const TransmissionSubgraph &)` — the
 * helper takes the subgraph rather than the analysis directly so it can use
 * `subgraph.requested_inputs()` to filter out trivially-already-supplied alternatives, and reach
 * into both the transmission inverse index and the affine group index on the underlying analysis.
 */
struct MissingInputResolution {
  /// The unreachable state interface this entry is for.
  StateInterfaceId missing = 0;

  /// Each entry is a set of state interfaces that — if all supplied — would unblock the missing
  /// interface via one specific transmission. Empty if no transmission path exists. Multiple
  /// entries indicate multiple alternative transmissions that could be used.
  std::vector<std::vector<StateInterfaceId>> transmission_alternatives{};

  /// If the missing interface lives in a non-trivial affine group AND that interface id has a
  /// registered `AffineProjectionRule`, this is the root joint of that group. The user resolves
  /// the missing interface by supplying any single `(joint_in_group, missing.interface_id)`
  /// where `joint_in_group` is any joint reported by `analysis.affine_group_members(affine_root)`.
  /// `std::nullopt` if no affine resolution is possible.
  std::optional<JointId> affine_root{};
};

/**
 * Computes resolution hints for the unreachable outputs in a `TransmissionSubgraph`.
 *
 * Returns one `MissingInputResolution` per interface in `subgraph.unreachable_outputs()`,
 * enumerating both transmission-based and affine-group-based resolution paths. Returns an empty
 * vector if the subgraph has no unreachable outputs.
 *
 * The helper takes the subgraph (not just the analysis) so it can:
 * - Use `subgraph.requested_inputs()` to filter out trivially-already-supplied alternatives.
 * - Surface only the alternatives that would actually unblock progress in the current request
 *   context.
 * - Reach into both the transmission inverse index and the affine group index on the underlying
 *   analysis.
 *
 * \note Stub implementation in step 3 of the state-interface refactor — returns an empty vector
 * regardless of input. Step 6 will give it the real algorithm when `DefaultJointMapBuilder` needs
 * it for rich error reporting.
 */
[[nodiscard]] std::vector<MissingInputResolution> compute_missing_input_resolutions(
  const TransmissionSubgraph & subgraph);

}  // namespace arm_kinematics

#endif  // ARM_KINEMATICS_MISSING_INPUT_RESOLUTION_HPP
