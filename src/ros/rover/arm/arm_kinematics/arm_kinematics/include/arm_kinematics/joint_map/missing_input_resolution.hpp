//
// Created by Bailey Chessum on 8/4/26.
//

#ifndef ARM_KINEMATICS_MISSING_INPUT_RESOLUTION_HPP
#define ARM_KINEMATICS_MISSING_INPUT_RESOLUTION_HPP

#include <optional>
#include <vector>

#include "arm_kinematics/joint_map/transmission_types.hpp"
#include "arm_kinematics/utilities/span.hpp"

namespace arm_kinematics {

class TransmissionReachability;  // forward declaration; full def in transmission_reachability.hpp

/**
 * One way to resolve a single missing (unreachable) state interface in a request. The builder
 * packages a vector of these into `JointMapBuildError::resolutions` so the user sees actionable
 * suggestions for unblocking the build.
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
 * Diagnosis of a request against a `TransmissionReachability`: which needed outputs are
 * unreachable, which are ambiguous, and (optionally) what the user could supply to fix them.
 *
 * Builders should treat the request as buildable iff `unreachable.empty() && ambiguous_outputs.empty()`.
 */
struct MissingOutputDiagnosis {
  /// Outputs that have no producer in the reachability (neither leaf, transmission, nor affine).
  std::vector<StateInterfaceId> unreachable;
  /// Outputs whose producer is ambiguous in the reachability — sliced from
  /// `reachability.ambiguities()` and intersected with `needed_outputs`.
  std::vector<StateInterfaceId> ambiguous_outputs;
  /// Resolution hints for each entry in `unreachable`. Same length as `unreachable`. Empty in the
  /// stub implementation; populated for real once `compute_missing_input_resolutions` is wired up.
  std::vector<MissingInputResolution> resolutions;
};

/**
 * Walks `needed_outputs` against `reach`, classifying each output as satisfied, unreachable, or
 * ambiguous, and packaging the unreachable/ambiguous slices into a diagnosis. Cheap O(N) — does
 * not run the resolution algorithm itself.
 *
 * Pure function; safe for concurrent use.
 */
[[nodiscard]] MissingOutputDiagnosis diagnose_missing_outputs(
  const TransmissionReachability & reach,
  span<const StateInterfaceId> needed_outputs);

/**
 * Computes resolution hints for the given missing interfaces against `reach`. Returns one
 * `MissingInputResolution` per entry in `missing`, in the same order, enumerating both
 * transmission-based and affine-group-based resolution paths.
 *
 * The function takes the reachability (not just the analysis) so it can use
 * `reach.inputs()` to filter out trivially-already-supplied alternatives, and reach into the
 * transmission inverse index and the affine group index on the underlying analysis.
 *
 * \note Stub implementation — returns one default-constructed entry per missing interface. The
 * real algorithm is deferred to step 6 when `DefaultJointMapBuilder` needs it for rich error
 * reporting.
 */
[[nodiscard]] std::vector<MissingInputResolution> compute_missing_input_resolutions(
  const TransmissionReachability & reach,
  span<const StateInterfaceId> missing);

}  // namespace arm_kinematics

#endif  // ARM_KINEMATICS_MISSING_INPUT_RESOLUTION_HPP
