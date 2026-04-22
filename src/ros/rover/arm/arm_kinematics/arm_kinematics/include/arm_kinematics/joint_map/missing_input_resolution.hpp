//
// Created by Bailey Chessum on 8/4/26.
//

#ifndef ARM_KINEMATICS_MISSING_INPUT_RESOLUTION_HPP
#define ARM_KINEMATICS_MISSING_INPUT_RESOLUTION_HPP

#include <optional>
#include <vector>

#include "arm_kinematics/joint_map/state_interface_producer.hpp"
#include "arm_kinematics/joint_map/transmission_types.hpp"
#include "arm_kinematics/utilities/span.hpp"

namespace arm_kinematics { class TransmissionReachability; }  // forward declaration

namespace arm_kinematics {

/**
 * One way to resolve a single missing (unproducible) state interface in a request. The builder
 * packages a vector of these into `JointMapBuildError::resolutions` so the user sees actionable
 * suggestions for unblocking the build.
 */
struct MissingInputResolution {
  /// The unproducible state interface this entry is for.
  StateInterfaceDefinition missing{};

  /// Each entry is a set of state interfaces that — if all supplied — would unblock the missing
  /// interface via one specific transmission. Empty if no transmission path exists. Multiple
  /// entries indicate multiple alternative transmissions that could be used.
  std::vector<std::vector<StateInterfaceDefinition>> transmission_alternatives{};

  /// If the missing interface lives in a non-trivial affine group AND that interface id has a
  /// registered `AffineProjectionRule`, this is the root joint of that group. The user resolves
  /// the missing interface by supplying any single `(joint_in_group, missing.interface_id)`
  /// where `joint_in_group` is any joint reported by `analysis.affine_group_members(affine_root)`.
  /// `std::nullopt` if no affine resolution is possible.
  std::optional<JointId> affine_root{};
};

/// Used for passing resolutions back to the user surface for easy logging
struct NamedMissingInputResolution {
  NamedStateInterfaceDefinition missing;
  std::vector<std::vector<NamedStateInterfaceDefinition>> transmission_alternatives{};
  std::optional<JointId> affine_root{};
};

/**
 * Diagnosis of a request against a `TransmissionReachability`. Each needed output falls into
 * exactly one of four cases:
 *
 *   - **Satisfied** — `reach.producer_of(out)` returns a non-monostate value. Not reported.
 *   - **Unproducible** — the user hasn't supplied enough inputs. None of the output's potential
 *     producers have all-derivable inputs. Listed in `unproducible`. Remediation: supply more
 *     inputs (see `resolutions`).
 *   - **Directly ambiguous** — the output itself has ≥2 viable producers in the analysis. The
 *     user authored multiple ways to compute this value and the algorithm refuses to silently
 *     pick. Listed in `directly_ambiguous_outputs`. Remediation: disambiguate the output by
 *     removing/disabling the unwanted producers.
 *   - **Transitively blocked** — the output's potential producer chain transitively depends on
 *     an ambiguous (or further transitively blocked) interface. Listed in
 *     `transitively_blocked_outputs`. Remediation: disambiguate the upstream interfaces in
 *     `relevant_blocking_ambiguities`.
 *
 * Builders treat the request as buildable iff `ok()` returns true (all three failure lists
 * empty).
 *
 * The classification is a simple O(N) pass over `needed_outputs` against the reachability's
 * pre-computed `derivable_interfaces()`, `ambiguities()`, and `transitively_blocked_interfaces()`
 * sets. The "which upstream ambiguities are responsible" attribution walk only runs for failing
 * outputs (and walks the analysis's potential-producer graph + affine group members).
 */
struct MissingOutputDiagnosis {
  /// Outputs that cannot be derived from the supplied inputs because the user hasn't supplied
  /// enough — none of their potential producers have all-derivable inputs. Remediation:
  /// supply more inputs.
  std::vector<StateInterfaceDefinition> unproducible;

  /// Outputs that have ≥2 viable producers in the analysis (the user authored multiple ways
  /// to compute this exact value). Remediation: pick one and remove/disable the others.
  std::vector<StateInterfaceDefinition> directly_ambiguous_outputs;

  /// Outputs whose potential producer chain transitively depends on an ambiguous (or further
  /// transitively blocked) interface. The output itself doesn't have multiple producers, but
  /// its only paths to derivability all go through an ambiguous upstream. Remediation:
  /// disambiguate the upstream entries in `relevant_blocking_ambiguities`.
  std::vector<StateInterfaceDefinition> transitively_blocked_outputs;

  /// The subset of `reach.ambiguities()` that some failing output (directly ambiguous OR
  /// transitively blocked) actually depends on. For directly-ambiguous outputs the dependency
  /// is the output itself. For transitively blocked outputs, the diagnose walks the analysis's
  /// potential-producer graph and affine group members to attribute the upstream ambiguities.
  std::vector<producers::AmbiguousInterface> relevant_blocking_ambiguities;

  /// Resolution hints for each entry in `unproducible`. Same length as `unproducible`.
  std::vector<MissingInputResolution> resolutions;

  /// Convenience: returns true iff the request is buildable (no unproducible, directly
  /// ambiguous, or transitively blocked outputs). Builders gate on this.
  [[nodiscard]] bool ok() const noexcept
  {
    return unproducible.empty() &&
           directly_ambiguous_outputs.empty() &&
           transitively_blocked_outputs.empty();
  }
};

/**
 * Walks `needed_outputs` against `reach`, classifying each output as satisfied, unproducible,
 * directly ambiguous, or transitively blocked, and packaging the failing slices into a
 * diagnosis. Cheap O(N) — does not run the resolution algorithm itself.
 *
 * Pure function; safe for concurrent use.
 */
[[nodiscard]] MissingOutputDiagnosis diagnose_missing_outputs(
  const TransmissionReachability & reach,
  span<const StateInterfaceDefinition> needed_outputs);

/**
 * Computes resolution hints for the given missing interfaces against `reach`. Returns one
 * `MissingInputResolution` per entry in `missing`, in the same order, enumerating both
 * transmission-based and affine-group-based resolution paths.
 *
 * The function takes the reachability (not just the analysis) so it can use
 * `reach.inputs()` to filter out trivially-already-supplied alternatives, and reach into the
 * transmission inverse index and the affine group index on the underlying analysis.
 */
[[nodiscard]] std::vector<MissingInputResolution> compute_missing_input_resolutions(
  const TransmissionReachability & reach,
  span<const StateInterfaceDefinition> missing);

}  // namespace arm_kinematics

#endif  // ARM_KINEMATICS_MISSING_INPUT_RESOLUTION_HPP
