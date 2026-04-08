//
// Created by Bailey Chessum on 8/4/26.
//

#ifndef ARM_KINEMATICS_AFFINE_PROJECTION_RULE_HPP
#define ARM_KINEMATICS_AFFINE_PROJECTION_RULE_HPP

namespace arm_kinematics {

/**
 * Encodes how a joint-level affine relationship (e.g. a mimic joint) should be projected onto a specific
 * state interface type (e.g. position, velocity, effort).
 *
 * Given an `AffineTransmission { source_joint_id, target_joint_id, multiplier, offset }`, the projected
 * affine relationship for a particular interface is:
 * \code
 *   projected_target_iface = effective_m * projected_source_iface + effective_o
 * \endcode
 * where:
 * \code
 *   effective_m = multiplier * multiplier_scale
 *   effective_o = offset * offset_scale
 * \endcode
 * If `reverse_direction` is true, the source/target roles swap before applying — i.e.
 * `projected_source_iface` corresponds to `AffineTransmission::target_joint_id`'s interface, and
 * `projected_target_iface` corresponds to `AffineTransmission::source_joint_id`'s interface.
 *
 * Standard projections:
 * - Position: `{ multiplier_scale=1, offset_scale=1, reverse_direction=false }` → `q_b = m·q_a + o`
 * - Velocity: `{ multiplier_scale=1, offset_scale=0, reverse_direction=false }` → `v_b = m·v_a`
 * - Effort   : `{ multiplier_scale=1, offset_scale=0, reverse_direction=true  }` → `τ_a = m·τ_b`
 *   (energy-conserving; opt-in only — see TransmissionAnalysis defaults)
 */
struct AffineProjectionRule {
  float multiplier_scale = 1.0F;
  float offset_scale = 1.0F;
  bool reverse_direction = false;
};

} // namespace arm_kinematics

#endif // ARM_KINEMATICS_AFFINE_PROJECTION_RULE_HPP
