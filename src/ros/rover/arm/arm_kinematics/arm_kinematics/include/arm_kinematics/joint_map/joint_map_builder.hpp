//
// Created by Bailey Chessum on 21/03/2026.
//

#ifndef ARM_KINEMATICS_JOINT_MAP_BUILDER_HPP
#define ARM_KINEMATICS_JOINT_MAP_BUILDER_HPP

#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "arm_kinematics/joint_map/joint_map.hpp"
#include "arm_kinematics/joint_map/missing_input_resolution.hpp"
#include "arm_kinematics/joint_map/state_interface_definition.hpp"
#include "arm_kinematics/joint_map/state_interface_producer.hpp"
#include "arm_kinematics/joint_map/transmission_types.hpp"
#include "arm_kinematics/utilities/expected.hpp"
#include "arm_kinematics/utilities/span.hpp"
#include "arm_kinematics/visibility_control.h"

namespace arm_kinematics {

/**
 * Error returned by `JointMapBuilder::build_expected` when a request cannot be satisfied.
 *
 * The error is a tagged `std::variant` with one payload per failure mode. This avoids the
 * old "enum discriminator + mostly-empty fields" shape and makes the compiler enforce which
 * payload exists for a given failure.
 */
struct JointMapBuildError {
  struct MissingInputs {
    std::vector<StateInterfaceDefinition> unproducible_outputs{};
    std::vector<MissingInputResolution> resolutions{};

    [[nodiscard]] std::string format() const;
  };

  struct Ambiguous {
    std::vector<producers::AmbiguousInterface> ambiguous_interfaces{};

    [[nodiscard]] std::string format() const;
  };

  struct UnknownJoint {
    std::vector<JointId> unknown_joints{};

    [[nodiscard]] std::string format() const;
  };

  using Variant = std::variant<MissingInputs, Ambiguous, UnknownJoint>;
  Variant value;

  template <
    typename T,
    typename Decayed = std::decay_t<T>,
    typename = std::enable_if_t<!std::is_same_v<Decayed, JointMapBuildError>>>
  JointMapBuildError(T && t)
  : value(std::forward<T>(t))
  {
  }

  [[nodiscard]] std::string format() const;
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
   * The canonical request boundary uses `StateInterfaceDefinition` (`JointId` + `InterfaceId`):
   * callers resolve joint names against a `TransmissionAnalysis` via `joint_order()` to obtain
   * `JointId`s, then pair them with the desired `InterfaceId`.
   * `TransmissionAnalysis::StateInterfaceId` is an
   * implementation detail of the builder — callers never need to pre-register state interfaces.
   *
   * Any registered joint paired with any `InterfaceId` is a valid request. A `JointId` that is
   * not registered in the analysis returns `JointMapBuildError::UnknownJoint`.
   *
   * \warning Not real-time safe — performs allocation and graph analysis. Call once at setup
   * time and reuse the resulting `JointMap` at runtime.
   */
  [[nodiscard]] virtual tl::expected<JointMap, JointMapBuildError> build_expected(
    span<const StateInterfaceDefinition> inputs,
    span<const StateInterfaceDefinition> outputs) const = 0;
};

}  // namespace arm_kinematics

#endif  // ARM_KINEMATICS_JOINT_MAP_BUILDER_HPP
