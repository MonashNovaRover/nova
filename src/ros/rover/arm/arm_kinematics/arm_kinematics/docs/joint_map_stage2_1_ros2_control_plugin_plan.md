# JointMap Stage 2.1 ros2_control Plugin Plan

## Purpose

This note records the implementation plan for Stage 2.1 after the ros2_control transmission support direction was
corrected away from per-type local transmission math.

## Implemented

The codebase now has:

- `Ros2ControlTransmissionPluginLoader` as a reusable adapter around
  `transmission_interface::TransmissionLoader` pluginlib loading
- `RobotModel::get_ros2_control_transmission_plugin_loader()` as the lazy shared default cache for the default/shared
  analysis path
- ros2_control transmission import built on `hardware_interface::parse_control_resources_from_urdf(...)`
- one generic ros2_control-backed `TransmissionModel` wrapper
- one generic ros2_control-backed `ComputeTransmission` wrapper
- quantity-specific validation via real ros2_control `configure(...)` calls for:
  - `JointQuantity::Position`
  - `JointQuantity::Velocity`
- end-to-end grouped runtime coverage through the generic wrapper path for:
  - `transmission_interface/SimpleTransmission`
  - `transmission_interface/DifferentialTransmission`
- FK-plugin coverage reusing the shared default ros2_control-backed analysis path
- explicit failure coverage for an unknown ros2_control plugin type through the generic wrapper path

## Current Design Boundary

The current design is:

- ros2_control parsing and plugin loading are setup/build time only
- `TransmissionAnalysis` stores structural topology plus generic build-capability objects
- the grouped planner/compiler/runtime path remains ros2_control-agnostic
- the ros2_control-specific adapter layer is confined to:
  - `Ros2ControlTransmissionPluginLoader`
  - ros2_control-backed `TransmissionModel`
  - ros2_control-backed `ComputeTransmission`

## Status

Stage 2.1 is now substantially complete.

The generic ros2_control adapter layer is in place, the old per-type local transmission math is gone, and the default
shared path through `RobotModel` and `ForwardKinematicsPlugin` is covered.

## Next Stage

The next meaningful work should now come from the broader Stage 2 spec rather than more ros2_control adapter work.

The most important next step is:

1. move from “affine path or grouped path” selection toward structural mixed-plan composition
2. identify maximal affine-only portions of a request from cached analysis
3. compile each affine portion into one `AffineJointMap`
4. separate those affine portions by grouped non-affine stages only where actually required

If that starts forcing a large new runtime type surface, it is reasonable to stop at the first clean structural
representation and defer full automatic mixed execution to the next stage after that.

## Deferred

These should remain out of Stage 2.1 unless forced by a concrete need:

- automatic mixed affine/non-affine segmentation
- broadening `JointQuantity` beyond position/velocity
- exposing ros2_control types beyond the adapter layer
- moving grouped runtime semantics back into `DefaultJointMapBuilder`
