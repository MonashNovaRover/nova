//
// Created by Bailey Chessum on 8/4/26.
//

#ifndef ARM_KINEMATICS_MISSING_INPUT_RESOLUTION_HPP
#define ARM_KINEMATICS_MISSING_INPUT_RESOLUTION_HPP

#include <optional>
#include <vector>

#include "arm_kinematics/joint_map/transmission_reachability.hpp"
#include "arm_kinematics/joint_map/transmission_types.hpp"
#include "arm_kinematics/utilities/span.hpp"

namespace arm_kinematics {

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
 * Diagnosis of a request against a `TransmissionReachability`. Each needed output falls into
 * exactly one of four cases:
 *
 *   - **Satisfied** — `reach.producer_of(out)` returns a non-monostate value. Not reported.
 *   - **Unreachable** — the user simply hasn't supplied enough inputs. The output's potential
 *     producers all need other inputs that aren't derivable. Listed in `unreachable`.
 *   - **Directly ambiguous** — the output itself has ≥2 viable producers. Listed in
 *     `ambiguous_outputs`. The relevant ambiguity is the output's own entry in
 *     `reach.ambiguities()`.
 *   - **Blocked** — the output has at least one potential producer transmission, but every
 *     such producer transitively depends on an ambiguous (or further blocked) input. Listed in
 *     `ambiguous_outputs` (lumped with directly-ambiguous because builders treat them
 *     identically — fail with `Kind::Ambiguous`, ask the user to disambiguate). The upstream
 *     blocking ambiguities are surfaced in `relevant_blocking_ambiguities`.
 *
 * Builders treat the request as buildable iff `unreachable.empty() && ambiguous_outputs.empty()`.
 *
 * Unlike the old "transitive walk" implementation, this diagnosis is a simple O(N) classification
 * pass over `needed_outputs` against the reachability's pre-computed `derivable_interfaces()`,
 * `ambiguities()`, and `blocked_interfaces()` sets. The "find which upstream ambiguity blocks
 * this output" attribution walk only runs for blocked outputs (i.e., only on the failing path).
 */
struct MissingOutputDiagnosis {
  /// Outputs that cannot be derived from the supplied inputs because the user hasn't supplied
  /// enough — none of their potential producers have all-derivable inputs.
  std::vector<StateInterfaceId> unreachable;
  /// Outputs that cannot be produced because they are themselves ambiguous (≥2 producers) or
  /// because their potential producer chain transitively depends on an ambiguous interface.
  /// Both kinds are reported here because builders treat them identically — fail with
  /// `Kind::Ambiguous` and surface the relevant ambiguities so the user can disambiguate.
  std::vector<StateInterfaceId> ambiguous_outputs;
  /// The subset of `reach.ambiguities()` that some entry in `ambiguous_outputs` actually
  /// depends on. For directly-ambiguous outputs the dependency is the output itself. For
  /// blocked outputs, the diagnose walks the analysis's potential-producer graph to attribute
  /// the upstream ambiguities. Empty when `ambiguous_outputs` is empty.
  std::vector<TransmissionReachability::AmbiguousInterface> relevant_blocking_ambiguities;
  /// Resolution hints for each entry in `unreachable`. Same length as `unreachable`. Stubbed
  /// for now (one default-constructed entry per missing interface).
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
