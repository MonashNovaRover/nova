//
// Created by Bailey Chessum on 5/4/26.
//

#ifndef ARM_KINEMATICS_TRANSMISSION_SUBGRAPH_HPP
#define ARM_KINEMATICS_TRANSMISSION_SUBGRAPH_HPP

#include <cstddef>
#include <variant>
#include <vector>

#include "arm_kinematics/joint_map/transmission_types.hpp"

namespace arm_kinematics {

/**
 * Producer alternatives carried in `StateInterfaceProducer`.
 *
 * Each alternative carries only the data relevant to its case (no fat-struct anti-pattern with
 * always-empty fields). The variant `StateInterfaceProducer` selects between them.
 */
namespace producers {

/// The interface's value comes directly from one of the request's input slots.
struct Input {
  /// Position in the subgraph's effective inputs span (i.e. the index a builder uses to read
  /// from the JointMap's `inputs` array at runtime). Always points at the first occurrence of
  /// the interface in the input list — duplicates of the same interface do not change this
  /// index.
  std::size_t input_index = 0;
};

/// The interface's value is derived from another known leaf interface via a (possibly composed)
/// affine relationship. The `source` is **always** another producer that resolves to either
/// `Input` or `Transmission` in a single `producer_of()` step — never another `AffineProjection`.
/// The algorithm enforces this by preferring leaf interfaces when picking a source.
struct AffineProjection {
  /// The known leaf interface providing the underlying value.
  StateInterfaceId source = 0;
  /// Already-composed interface-space coefficient. Computed in O(1) at planning time from the
  /// per-joint flat affine relations stored in `TransmissionAnalysis::affine_transmissions_`
  /// and the projection rule for the relevant interface id.
  float multiplier = 1.0F;
  float offset = 0.0F;
};

/// The interface's value is produced by a selected `TransmissionInstance` (a block compute
/// that may produce multiple values together).
struct Transmission {
  TransmissionInstanceId instance_id = 0;
};

}  // namespace producers

/// What produces a given `StateInterfaceId` in the current plan?
///
/// `std::monostate` represents "not produced" — the interface is unreachable, ambiguous (in
/// which case the producer is intentionally hidden because the algorithm refuses to pick a
/// winner), or out-of-scope (not in the request's inputs/outputs and not transitively
/// reachable). Builders should check `TransmissionSubgraph::is_ambiguous()` before walking
/// outputs.
using StateInterfaceProducer = std::variant<
  std::monostate,
  producers::Input,
  producers::AffineProjection,
  producers::Transmission
>;

/**
 * Build-time analysis utility that captures the relevant subgraph of a `TransmissionAnalysis`
 * for a given `(inputs, outputs)` request, supports incremental modification, and answers
 * planning queries.
 *
 * \note This class is **not yet implemented** in step 3 of the state-interface refactor — only
 * its associated types (`producers::*`, `StateInterfaceProducer`, `AmbiguousInterface`) are
 * defined here so that `JointMapBuildError` and `compute_missing_input_resolutions` can refer
 * to them. The class members (constructor, queries, mutation, the forward fixed-point
 * algorithm) are added in step 4.
 */
class TransmissionSubgraph {
public:
  /// One ambiguous interface in the subgraph's plan, with the full list of competing producers
  /// that the algorithm refused to silently choose between.
  ///
  /// Accumulated across the entire algorithm pass — `ambiguous_interfaces()` returns *every*
  /// ambiguous interface, not just the first one encountered. Builders surface the full set so
  /// the user can fix all conflicts in one round-trip.
  struct AmbiguousInterface {
    StateInterfaceId interface = 0;
    std::vector<StateInterfaceProducer> candidates{};
  };

  // (Members deferred to step 4. The class currently exists only so other headers can refer
  //  to it via forward declaration / nested type lookup.)
};

}  // namespace arm_kinematics

#endif  // ARM_KINEMATICS_TRANSMISSION_SUBGRAPH_HPP
