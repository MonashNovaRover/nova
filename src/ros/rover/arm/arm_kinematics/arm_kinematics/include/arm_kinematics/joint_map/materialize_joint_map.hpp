//
// Created by Bailey Chessum on 8/4/26.
//

#ifndef ARM_KINEMATICS_MATERIALIZE_JOINT_MAP_HPP
#define ARM_KINEMATICS_MATERIALIZE_JOINT_MAP_HPP

#include "arm_kinematics/joint_map/joint_map.hpp"

namespace arm_kinematics {

class JointMapBlueprint;
class TransmissionAnalysis;

/**
 * Materializes a `JointMapBlueprint` into a runtime `JointMap`.
 *
 * Picks the simplest concrete runtime type per blueprint shape:
 *   - **Pure affine** (no transmission stages, at most one `InputAffineBatch` segment) →
 *     a standalone `AffineJointMap` whose sources index directly into the user's input vector.
 *   - **Mixed** (any transmission stages) → a `CompositeJointMap` with a value-table scratch
 *     buffer. Each `InputAffineBatch` segment becomes one `AffineJointMap` stage that reads
 *     from the scratch and writes its results directly to the composite's output buffer. Each
 *     `TransmissionStage` becomes one `TransmissionJointMap` stage that gathers from scratch,
 *     runs the compute kernel, and scatters into scratch (so downstream stages can read it)
 *     and/or directly into the output buffer (for transmission outputs the request asked for).
 *
 * The `analysis` parameter is used **only** to look up `TransmissionModel`s and call
 * `build(inputs, outputs)` to obtain the `ComputeTransmission` instances that the runtime
 * `TransmissionJointMap`s wrap. For a pure-affine blueprint, the analysis is unused (but still
 * required, to keep the API consistent).
 *
 * \pre Every state interface in `blueprint.segments()` must be valid against `analysis`.
 * \pre `blueprint` must come from a complete diagnosis (no unreachable / ambiguous outputs).
 *      Asserted in debug builds; behavior is unspecified otherwise.
 *
 * \throws std::invalid_argument on shape errors (e.g. transmission model returning a null
 *         compute, or unexpected state interface mismatches between blueprint and analysis).
 */
[[nodiscard]] JointMap materialize_joint_map(
  const JointMapBlueprint & blueprint,
  const TransmissionAnalysis & analysis);

}  // namespace arm_kinematics

#endif  // ARM_KINEMATICS_MATERIALIZE_JOINT_MAP_HPP
