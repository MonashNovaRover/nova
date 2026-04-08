//
// Created by Bailey Chessum on 8/4/26.
//

#include "arm_kinematics/joint_map/missing_input_resolution.hpp"

#include <algorithm>

#include "arm_kinematics/joint_map/transmission_reachability.hpp"

namespace arm_kinematics {

MissingOutputDiagnosis diagnose_missing_outputs(
  const TransmissionReachability & reach,
  const span<const StateInterfaceId> needed_outputs)
{
  MissingOutputDiagnosis diag{};

  // Build a quick lookup for the ambiguous interface set.
  const auto ambiguities = reach.ambiguities();

  for (const StateInterfaceId out : needed_outputs) {
    // Already satisfied?
    if (!std::holds_alternative<std::monostate>(reach.producer_of(out))) {
      continue;
    }
    // monostate could mean either ambiguous or genuinely unreachable. Check the ambiguity list.
    bool is_ambiguous = false;
    for (const auto & amb : ambiguities) {
      if (amb.interface == out) {
        is_ambiguous = true;
        break;
      }
    }
    if (is_ambiguous) {
      // Avoid duplicates if the same output appears twice in needed_outputs.
      if (std::find(diag.ambiguous_outputs.begin(), diag.ambiguous_outputs.end(), out) ==
          diag.ambiguous_outputs.end())
      {
        diag.ambiguous_outputs.push_back(out);
      }
    } else {
      if (std::find(diag.unreachable.begin(), diag.unreachable.end(), out) ==
          diag.unreachable.end())
      {
        diag.unreachable.push_back(out);
      }
    }
  }

  // Resolutions: stub. Step 6 will replace this with the real algorithm:
  //   - For each unreachable interface, walk `analysis.producing_transmissions(interface)` to
  //     enumerate transmission-based resolution paths, filtering out alternatives whose missing
  //     inputs are already supplied via `reach.inputs()`.
  //   - Look up the joint's affine group via `analysis.affine_root_of(joint)`; if non-trivial AND
  //     the interface id has a registered AffineProjectionRule, populate `affine_root`.
  diag.resolutions = compute_missing_input_resolutions(reach, diag.unreachable);

  return diag;
}

std::vector<MissingInputResolution> compute_missing_input_resolutions(
  const TransmissionReachability & reach,
  const span<const StateInterfaceId> missing)
{
  (void)reach;
  // Stub: return one default-constructed entry per missing interface so callers can iterate
  // structurally even though the contents aren't useful yet.
  std::vector<MissingInputResolution> result;
  result.reserve(missing.size());
  for (const StateInterfaceId m : missing) {
    MissingInputResolution entry{};
    entry.missing = m;
    result.push_back(std::move(entry));
  }
  return result;
}

}  // namespace arm_kinematics
