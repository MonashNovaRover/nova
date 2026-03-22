# JointMap Stage 2.1 ros2_control Plugin Support Spec

## Purpose

This document specifies the Stage 2.1 redesign for ros2_control transmission support.

Stage 2 established the important internal shapes:

- `TransmissionAnalysis` as cached structural analysis
- indexed request planning
- `TransmissionPlan` and `CompiledTransmissionPlan`
- `TransmissionJointMap` as the grouped runtime executor
- `ForwardKinematicsPlugin` as the authoritative seam for which `TransmissionAnalysis` is used

However, the current concrete ros2_control support direction is not the right long-term design if it requires one
local `ComputeTransmission` implementation per ros2_control transmission type.

ros2_control transmissions are already a plugin system.
Stage 2.1 should therefore redesign ros2_control support around wrapping that existing plugin system, rather than
reimplementing every transmission type inside `arm_kinematics`.

## Problem Statement

The wrong direction is:

- add one local `TransmissionModel` / `ComputeTransmission` pair for each ros2_control transmission type
- duplicate transmission math already implemented by ros2_control
- keep widening `arm_kinematics` whenever ros2_control adds or customizes a transmission plugin

That will not scale for:

- unknown third-party ros2_control transmission plugins
- private or controller-specific transmission plugins
- keeping behavior consistent with ros2_control's own implementation over time

The right direction is:

- keep `arm_kinematics` responsible for analysis, planning, and runtime orchestration
- let ros2_control transmission plugins remain responsible for transmission-specific math
- introduce a wrapper layer so the existing ros2_control plugin system can be used behind the `TransmissionModel` /
  `ComputeTransmission` boundary

## Goals

Stage 2.1 should:

1. support unknown ros2_control transmission plugin types without new `arm_kinematics` runtime classes per type
2. preserve the existing `TransmissionAnalysis -> TransmissionPlan -> CompiledTransmissionPlan -> TransmissionJointMap`
   architecture
3. keep all string parsing and plugin loading in setup/build time only
4. avoid heap allocation in the runtime hot path
5. keep affine transmission handling out of this path
6. keep ros2_control-specific types confined to a wrapper/adapter layer, not spread through core runtime planning
7. allow FK plugins to continue augmenting or replacing the analysis they expose

## Non-Goals

Stage 2.1 must not:

- move mimic or affine transmission semantics onto ros2_control
- degrade the `AffineJointMap` fast path
- require `TransmissionJointMap` itself to understand ros2_control XML or ros2_control transmission classes
- reintroduce string lookup into runtime execution
- require one `ComputeTransmission` subclass per ros2_control transmission type

## Core Design

The key design shift is:

- `TransmissionModel` remains the build-time capability interface
- ros2_control support is implemented by one generic wrapper model
- that wrapper model delegates transmission-specific math to ros2_control's existing transmission plugin instance

The reusable setup-time seam should be explicit too:

```cpp
class Ros2ControlTransmissionPluginLoader {
public:
  [[nodiscard]] std::vector<std::string> get_declared_plugin_types() const;
  [[nodiscard]] bool has_plugin_type(const std::string & plugin_type) const noexcept;
  [[nodiscard]] std::shared_ptr<transmission_interface::TransmissionLoader> make_loader(
    const std::string & plugin_type) const;
  [[nodiscard]] std::shared_ptr<transmission_interface::Transmission> load(
    const hardware_interface::TransmissionInfo & transmission_info) const;
};
```

And:

```cpp
class Ros2ControlPluginTransmissionModel final : public TransmissionModel {
public:
  Ros2ControlPluginTransmissionModel(
    hardware_interface::TransmissionInfo transmission_info,
    std::shared_ptr<const Ros2ControlTransmissionPluginLoader> plugin_loader);

  [[nodiscard]] std::unique_ptr<TransmissionModel> clone() const override;

  [[nodiscard]] bool can_build(
    JointQuantity quantity,
    PropagationDirection direction) const noexcept override;

  [[nodiscard]] std::unique_ptr<const ComputeTransmission> build(
    JointQuantity quantity,
    PropagationDirection direction,
    span<const JointId> input_joint_ids,
    span<const JointId> output_joint_ids) const override;
};
```

And:

```cpp
class Ros2ControlPluginTransmissionCompute final : public ComputeTransmission {
public:
  void compute(
    span<const float> inputs,
    span<float> outputs,
    span<float> scratch) const override;
};
```

This is still one local wrapper type, but it is not one local type per transmission math implementation. The
transmission-specific logic remains in the ros2_control plugin instance.

## Parsing Boundary

Stage 2.1 should stop manually reparsing ros2_control XML.

The ros2_control boundary should use:

- `hardware_interface::parse_control_resources_from_urdf(...)`
- `hardware_interface::HardwareInfo::transmissions`
- `hardware_interface::TransmissionInfo`

That keeps `arm_kinematics` on the supported ros2_control parsing path instead of maintaining a parallel XML parser for
transmission metadata.

## Architectural Boundary

Stage 2.1 should keep these responsibilities separate.

### `TransmissionAnalysis`

Owns structural facts only:

- canonical `JointId` assignment
- transmission group topology
- affine transmission relationships
- the build-capability objects referenced by transmission groups

It may store ros2_control-backed `TransmissionModel` wrapper instances, but it should not store:

- ros2_control runtime handles
- mutable runtime buffers
- pluginlib loader state that must be recreated per request

The reusable ros2_control plugin loader/cache should sit next to this layer rather than inside each model instance.
For the default/shared path, `RobotModel` should lazily provide that shared loader/cache in the same spirit as
`get_default_transmission_analysis()`.

### `TransmissionModel`

At build time, decides whether it can produce runtime compute for:

- a requested `JointQuantity`
- a requested `PropagationDirection`
- a specific group topology

For ros2_control-backed models, this means:

- load or construct a ros2_control transmission plugin instance
- verify the plugin can be wrapped for the requested quantity/direction
- build a generic runtime compute wrapper around that plugin instance

### `ComputeTransmission`

At runtime, performs grouped math on preallocated indexed buffers.

For ros2_control-backed compute wrappers, this means:

- copy caller-provided float inputs into preallocated handle storage
- invoke the wrapped ros2_control transmission plugin in the appropriate direction
- copy results back into the provided float outputs

No plugin loading, no XML parsing, and no heap allocation should occur in `compute(...)`.

## Wrapper Strategy

The wrapper should be generic over ros2_control transmission types.

### Build-Time Loader Path

At build time:

1. parse `hardware_interface::TransmissionInfo` using `hardware_interface::parse_control_resources_from_urdf(...)`
2. create one `Ros2ControlPluginTransmissionModel` in analysis for that transmission definition
3. when `build(...)` is called:
   - use the reusable ros2_control plugin loader/cache to instantiate the `TransmissionLoader` plugin named by the
     transmission info
   - ask it to construct a `transmission_interface::Transmission`
   - prepare handle layouts for the requested `JointQuantity`
   - create one runtime wrapper compute object around that constructed transmission instance

This preserves the ros2_control plugin system as the authority for transmission-specific construction.

### Runtime Wrapper Path

The runtime wrapper should pre-own all data needed for repeated compute:

- the constructed `transmission_interface::Transmission`
- ordered actuator/joint handle arrays
- quantity-specific backing storage
- any scratch arrays required by the wrapper

At runtime, the wrapper should only:

1. copy input span values into the preallocated actuator-space or joint-space backing storage
2. call either actuator-to-joint or joint-to-actuator on the wrapped transmission
3. copy produced values into the output span

That is acceptable even if it is not the mathematically cheapest possible path, because:

- it preserves correct ros2_control plugin semantics
- it avoids per-type reimplementation
- it keeps allocation out of the hot path

## Quantity Handling

Stage 2.1 should keep quantity build-time only.

For ros2_control wrappers:

- `JointQuantity::Position` should bind the plugin wrapper to position handles only
- `JointQuantity::Velocity` should bind the plugin wrapper to velocity handles only
- unsupported quantities should fail clearly in `can_build(...)` / `build(...)`

The runtime wrapper should not branch on quantity except insofar as the built wrapper already owns the chosen handle
set.

That means different quantities may still produce different wrapper compute objects, but not different public runtime
APIs.

## Direction Handling

Direction remains a build-time concern too.

For ros2_control wrappers:

- forward means caller inputs are actuator-space values and outputs are joint-space values
- reverse means caller inputs are joint-space values and outputs are actuator-space values

The built runtime wrapper should know which ros2_control method to call:

- actuator-to-joint
- joint-to-actuator

`TransmissionPlanStage` and `CompiledTransmissionStage` already model this cleanly.

## Data Layout Expectations

The wrapper should respect the already-planned grouped topology.

That means:

- `TransmissionAnalysis` remains the source of canonical group membership
- `TransmissionPlanStage` still says which `JointId`s are consumed and produced
- the ros2_control wrapper must only be built for plans whose grouped topology matches the wrapped transmission

No runtime string lookup should occur here.
Joint ordering for ros2_control handles should be fixed at build time from the already-indexed group topology.

## Error Semantics

Stage 2.1 should fail clearly when:

- the ros2_control transmission plugin class cannot be loaded
- the plugin cannot construct a transmission from the given `TransmissionInfo`
- the requested quantity cannot be configured on the wrapped transmission
- the requested direction is not supported
- the planner's grouped topology does not match the wrapped transmission's actuator/joint arity

These failures belong at build time, not runtime.

## Scope Of ros2_control Types

Stage 2.1 should narrow ros2_control usage to:

- parsing `hardware_interface::TransmissionInfo`
- build-time wrapper construction
- runtime wrapper internals that own a constructed ros2_control transmission instance

ros2_control types should not spread into:

- `TransmissionPlan`
- `CompiledTransmissionPlan`
- `TransmissionJointMap`
- affine planning
- general builder selection logic

That keeps the core architecture reusable.

## Testing Requirements

Minimum Stage 2.1 coverage should include:

1. one ros2_control `SimpleTransmission` built through the generic wrapper path
2. one ros2_control grouped transmission built through the generic wrapper path
3. forward position mapping through the wrapper
4. reverse position mapping through the wrapper
5. forward velocity mapping through the wrapper
6. reverse velocity mapping through the wrapper
7. failure when the named ros2_control transmission plugin cannot be loaded
8. failure when quantity/direction is unsupported by the wrapper path
9. proof that runtime compute reuses preallocated backing storage
10. proof that FK plugins can still augment shared analysis while reusing wrapped ros2_control models

These tests should be treated as adapter-layer coverage.
Core planning/runtime tests should stay independent of ros2_control where practical.

## Migration Plan

Recommended Stage 2.1 sequence:

1. Introduce a reusable `Ros2ControlTransmissionPluginLoader` wrapper around
   `transmission_interface::TransmissionLoader` pluginlib loading.
2. Introduce a generic ros2_control-backed `TransmissionModel` wrapper type.
3. Introduce a generic ros2_control-backed `ComputeTransmission` wrapper type.
4. Update ros2_control import to use `hardware_interface::parse_control_resources_from_urdf(...)`.
5. Update ros2_control import to create the generic wrapper model instead of per-type local compute/model classes.
6. Keep existing `TransmissionAnalysis`, planner, compiler, and `TransmissionJointMap` unchanged where possible.
7. Re-express current ros2_control tests so they validate the generic wrapper path rather than local one-off implementations.
8. Remove any one-local-type-per-ros2-control-transmission implementation that is now redundant.

## Acceptance Criteria

Stage 2.1 is complete when:

- ros2_control support no longer depends on one `ComputeTransmission` implementation per transmission type
- unknown ros2_control transmission plugin types can be supported through the generic wrapper path, assuming the plugin
  is installed and compatible
- the runtime grouped execution path still remains allocation-free in routine use
- the existing Stage 2 planning/runtime architecture remains intact
- affine transmission handling remains separate and optimized
- ros2_control remains an adapter layer, not the center of the core architecture

## Relationship To Later Stages

Stage 2.1 should make later work easier, not harder.

In particular it should prepare for:

- additional ros2_control transmission plugins without new runtime type sprawl
- plugin-specific FK analysis augmentation
- eventual mixed affine/non-affine segmentation without changing ros2_control integration again

This is a redesign of the ros2_control adapter layer, not a redesign of the full Stage 2 planner/runtime architecture.
