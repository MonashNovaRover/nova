# JointMap Stage 2 Grouped Runtime Plan

## Purpose

This note records the grouped-runtime implementation step after the affine transmission path was moved onto:

- `TransmissionAnalysis` for cached structure
- `make_affine_plan_expected(...)` for affine planning
- `AffineJointMap` for runtime affine execution

The grouped transmission runtime path was the major gap addressed by this note.

## Current State

The codebase now has:

- cached grouped transmission structure in `TransmissionAnalysis`
- cached affine transmission structure in `TransmissionAnalysis`
- copyable `TransmissionAnalysis` for FK-plugin-local reuse or augmentation
- grouped planning over cached grouped transmissions in indexed space
- a working affine planner and affine runtime compilation path
- grouped compiler validation for:
  - stage topology
  - stage data availability
  - unsupported model build directions/quantities
- grouped ambiguity handling across multiple candidate plans
- builder-level grouped orchestration through `DefaultJointMapBuilder`
- builder-level grouped execution and copy-semantics test coverage
- `ForwardKinematicsPlugin` as the authoritative seam for the `TransmissionAnalysis` its builders consume
- `RobotModel` reduced to a lazy shared default `TransmissionAnalysis` cache
- builder cache invalidation when an FK plugin switches which `TransmissionAnalysis` object it exposes

What is still missing is the grouped runtime execution side:

- no major grouped runtime execution gap remains in this stage slice

## Current Runtime Scope

The codebase now has:

1. `CompiledTransmissionStage`
2. `CompiledTransmissionPlan`
3. `TransmissionJointMap`
4. a compiler from `TransmissionPlan` to `CompiledTransmissionPlan`
5. scratch sizing via `ComputeTransmission::scratch_size()`

This runtime path now supports:

- multi-stage grouped plans derived from `TransmissionAnalysis`
- manually constructed multi-stage grouped plans at compile/runtime level
- stage-to-stage dataflow through a compiled value buffer
- builder-produced grouped `JointMap` instances
- grouped `JointMap` copy semantics through cloned `ComputeTransmission` stages

## Design Constraints

- `TransmissionModel` remains the build-time capability interface only
- `ComputeTransmission` remains the runtime grouped compute interface
- affine transmission and mimic handling stay out of this path and should continue to compile into `AffineJointMap`
- many affine transmission relationships should collapse into one `AffineJointMap` per affine execution segment rather
  than executing as separate runtime stages
- runtime execution should not allocate
- runtime execution should not do string lookup
- `DefaultJointMapBuilder` should not become the design center for this work

## Status

This grouped runtime plan is now substantially complete.

The reusable grouped planner/compiler/runtime structures exist, and `DefaultJointMapBuilder` is now acting as a thin
consumer of those structures rather than owning grouped semantics.

The FK-plugin ownership migration is also complete:

- `RobotModel` exposes only `get_default_transmission_analysis()`
- `ForwardKinematicsPlugin::get_transmission_analysis()` is the authoritative seam
- `ForwardKinematicsPlugin::get_joint_map_builder()` now consumes that seam and rebuilds its cached builder if the
  exposed analysis object changes

## Next Stage

The next work should come from the broader Stage 2 / transmission-spec documents rather than from this focused grouped
runtime note.

The most important remaining items are:

1. make quantity-specific build behavior real for concrete transmission models rather than only structurally supported
2. broaden real transmission-model coverage beyond simple single-input single-output transmissions
3. decide which additional ros2_control transmission forms should receive real grouped runtime implementations next
4. decide whether any reusable concrete `TransmissionModel` implementations should be promoted out of import-time
   internals
5. add plugin-extension coverage where FK plugins augment, rather than only replace, the shared default analysis

Longer-term direction beyond this focused note:

- mixed requests should eventually be partitioned automatically into maximal affine execution segments separated by
  non-affine grouped stages
- each affine-only segment should compile into one `AffineJointMap`
- that mixed-stage composition should not be overbuilt into Stage 2 if doing so would distort the simpler current
  architecture
