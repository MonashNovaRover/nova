//
// Created by Bailey Chessum on 21/03/2026.
//

#ifndef ARM_KINEMATICS_JOINT_MAP_BUILDER_HPP
#define ARM_KINEMATICS_JOINT_MAP_BUILDER_HPP

#include <string>
#include <vector>

#include "arm_kinematics/joint_map/joint_map.hpp"
#include "arm_kinematics/joint_map/missing_input_resolution.hpp"
#include "arm_kinematics/joint_map/transmission_reachability.hpp"
#include "arm_kinematics/joint_map/transmission_types.hpp"
#include "arm_kinematics/utilities/expected.hpp"
#include "arm_kinematics/utilities/span.hpp"
#include "arm_kinematics/visibility_control.h"

namespace arm_kinematics {

/**
 * Error returned by `JointMapBuilder::build_expected` when a request cannot be satisfied.
 *
 * Three failure modes are surfaced:
 * - **MissingInputs:** one or more requested outputs are not derivable from the supplied inputs.
 *   `unreachable_outputs` lists which outputs failed; `resolutions` carries actionable hints
 *   from `compute_missing_input_resolutions`.
 * - **Ambiguous:** one or more requested outputs have multiple viable producers (or
 *   transitively depend on something that does) and the reachability algorithm refuses to
 *   silently pick a winner. `ambiguous_interfaces` lists the upstream conflicts the user must
 *   resolve.
 * - **UnknownInterface:** one or more `StateInterfaceId`s in the request are out of range for
 *   the analysis (i.e., the caller passed a stale or fabricated id). `unknown_interfaces`
 *   lists the bad ids. This is distinct from `MissingInputs` because the user's bug is "I
 *   passed an id that doesn't exist", not "I forgot to supply some inputs".
 *
 * **Precedence when multiple failure modes apply.** UnknownInterface is checked first
 * (validation is cheap and a stale id makes everything else moot). Then Ambiguous wins over
 * MissingInputs — the user fixes ambiguities first and retries.
 */
struct JointMapBuildError {
  enum class Kind {
    /// Needed outputs are not derivable from the given inputs.
    MissingInputs,
    /// One or more needed interfaces have multiple viable producers (directly or transitively).
    Ambiguous,
    /// One or more `StateInterfaceId`s in the request are out of range for the analysis.
    UnknownInterface,
  };
  Kind kind = Kind::MissingInputs;

  /// Human-readable message. Should reference `TransmissionInstance::name` when identifying
  /// transmissions in ambiguity reports or resolution hints, so users see
  /// "transmission `differential_left`" rather than "transmission 3".
  std::string message{};

  /// Populated when `kind == MissingInputs`. Each entry is a needed output that the algorithm
  /// could not derive from the requested inputs. Empty when `kind == Ambiguous`.
  std::vector<StateInterfaceId> unreachable_outputs{};

  /// Populated when `kind == MissingInputs`. Optional rich-error hints — one entry per
  /// unreachable output — describing what could be supplied to unblock it. May be empty.
  std::vector<MissingInputResolution> resolutions{};

  /// Populated when `kind == Ambiguous`. Each entry is one ambiguous interface that the
  /// requested outputs transitively depend on (directly or via the producer chain). Sliced
  /// from the reachability's full ambiguity list — unrelated ambiguities elsewhere in the
  /// analysis are not reported.
  std::vector<TransmissionReachability::AmbiguousInterface> ambiguous_interfaces{};

  /// Populated when `kind == UnknownInterface`. Each entry is a `StateInterfaceId` from the
  /// request that is out of range for the analysis's state interface order.
  std::vector<StateInterfaceId> unknown_interfaces{};
};

/**
 * Builder interface for constructing `JointMap` instances.
 *
 * A builder is responsible for translating a request — "given these inputs, produce these
 * outputs" — into a runtime `JointMap` that performs the mapping at compute time. The default
 * implementation (`DefaultJointMapBuilder`) constructs a `TransmissionReachability` and a
 * `JointMapBlueprint` to plan and emit the runtime joint map, but FK plugins may return
 * specialized builders from
 * `ForwardKinematicsPlugin::get_joint_map_builder()` to express backend-specific mapping
 * policies (e.g. supplying default values for missing inputs, applying custom transformations,
 * etc.).
 *
 * Strategy for handling missing inputs lives entirely in subclasses (open/closed principle).
 * The base interface only commits to the request shape and the failure modes.
 *
 * \note **Duplicate output state interfaces are allowed in builder requests** (asking for the
 * same value in two output positions is legitimate, e.g. fan-out into multiple consumers) and
 * are not an error.
 */
class ARM_KINEMATICS_PUBLIC JointMapBuilder {
public:
  virtual ~JointMapBuilder() = default;

  /**
   * Constructs a `JointMap` that maps the given input state interfaces to the given output
   * state interfaces.
   *
   * The canonical request boundary uses `StateInterfaceId` (not `StateInterfaceDefinition` and
   * not joint names): callers resolve names against a `TransmissionAnalysis` up front via
   * `ensure_state_interface_id` and pass the resulting ids here. This keeps the builder API
   * minimal and unambiguous.
   *
   * \warning Not real-time safe — performs allocation and graph analysis. Call once at setup
   * time and reuse the resulting `JointMap` at runtime.
   */
  [[nodiscard]] virtual tl::expected<JointMap, JointMapBuildError> build_expected(
    span<const StateInterfaceId> inputs,
    span<const StateInterfaceId> outputs) const = 0;
};

}  // namespace arm_kinematics

#endif  // ARM_KINEMATICS_JOINT_MAP_BUILDER_HPP
