//
// Created by Bailey Chessum on 8/4/26.
//

#include "arm_kinematics/joint_map/missing_input_resolution.hpp"

#include "arm_kinematics/joint_map/transmission_subgraph.hpp"

namespace arm_kinematics {

// Stub implementation. Step 6 will replace this with the real algorithm:
//   - Iterate `subgraph.unreachable_outputs()`
//   - For each unreachable interface, walk `analysis.producing_transmissions(interface)` to
//     enumerate transmission-based resolution paths, filtering out alternatives that are
//     already trivially supplied via `subgraph.requested_inputs()`
//   - Look up the joint's affine group via `analysis.affine_root_of(joint)`; if non-trivial AND
//     the interface id has a registered AffineProjectionRule, populate `affine_root`
//   - Skip transmission alternatives whose required inputs are themselves already supplied
//
// For now, returning empty so DefaultJointMapBuilder can be wired up in step 6 against the real
// signature without blocking on the resolution algorithm itself.
std::vector<MissingInputResolution> compute_missing_input_resolutions(const TransmissionSubgraph & subgraph)
{
  (void)subgraph;
  return {};
}

}  // namespace arm_kinematics
