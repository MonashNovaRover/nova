//
// Created by Bailey Chessum on 12/4/26.
//

#ifndef ARM_KINEMATICS_STATE_INTERFACE_PRODUCER_HPP
#define ARM_KINEMATICS_STATE_INTERFACE_PRODUCER_HPP

#include <cstddef>
#include <variant>
#include <vector>

#include "arm_kinematics/joint_map/state_interface_definition.hpp"
#include "arm_kinematics/joint_map/transmission_types.hpp"

namespace arm_kinematics {

/**
 * Producer alternatives and the variant type that wraps them.
 *
 * Defined here — outside `TransmissionReachability` — so that `JointMapBuildError` (public API)
 * can refer to these types without pulling in the full `TransmissionReachability` header and its
 * heavy `TransmissionAnalysis` dependency.
 */
namespace producers {

/// The interface's value comes directly from one of the analysis-input slots.
struct Input {
  /// Position in the reachability's effective inputs span (i.e. the index a builder uses to read
  /// from the JointMap's `inputs` array at runtime). Always points at the first occurrence of
  /// the interface in the input list — duplicates of the same interface do not change this
  /// index.
  std::size_t input_index = 0;
};

/// The interface's value is derived from another known leaf interface via a (possibly composed)
/// affine relationship. The `source` is **always** another producer that resolves to either
/// `Input` or `Transmission` in a single `producer_of_def()` step — never another
/// `AffineProjection`. The algorithm enforces this by preferring leaf interfaces when picking
/// a source.
///
/// `source` is a `StateInterfaceDefinition` (not a `StateInterfaceId`) because affine
/// transmissions are purely joint-level: `joint_B = k * joint_A + offset` for every interface,
/// regardless of whether `(joint_A, interface)` has a registered `StateInterfaceId`. A bare
/// input (no SID) can therefore serve as the affine source without any pre-registration.
struct AffineProjection {
  /// The known leaf interface providing the underlying value. May be a registered
  /// `(joint, interface)` pair (has a real `StateInterfaceId`) or a bare definition (no SID,
  /// sourced from a direct user input).
  StateInterfaceDefinition source{};
  /// Already-composed interface-space coefficient. Computed in O(1) at planning time from the
  /// per-joint flat affine relations stored in `TransmissionAnalysis::affine_transmissions_`
  /// and the projection rule for the relevant interface id.
  double multiplier = 1.0;
  double offset = 0.0;
};

/// The interface's value is produced by a selected `TransmissionInstance` (a block compute
/// that may produce multiple values together).
struct Transmission {
  TransmissionInstanceId instance_id = 0;
};

/// What produces a given `StateInterfaceId` in the current reachability?
///
/// `std::monostate` represents "not produced" — the interface is unreachable from the analyzed
/// inputs, ambiguous (in which case the producer is intentionally hidden because the algorithm
/// refuses to pick a winner), or out-of-scope (not in the inputs and not transitively reachable).
using StateInterfaceProducer = std::variant<
  std::monostate,
  Input,
  AffineProjection,
  Transmission
>;

/// One ambiguous interface encountered during reachability analysis, with the full list of
/// competing producers that the algorithm refused to silently choose between.
///
/// Used in `JointMapBuildError::ambiguous_interfaces` (public API) and as the element type of
/// `TransmissionReachability::ambiguities()` (internal). Defined here — outside
/// `TransmissionReachability` — so that `JointMapBuildError` can reference it without pulling in
/// the full `TransmissionReachability` header.
struct AmbiguousInterface {
  StateInterfaceDefinition interface{};
  std::vector<StateInterfaceProducer> candidates{};
};

}  // namespace producers

}  // namespace arm_kinematics

#endif  // ARM_KINEMATICS_STATE_INTERFACE_PRODUCER_HPP
