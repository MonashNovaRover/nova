# JointMap Stage 2 Grouped Runtime Plan

## Purpose

This note records the next implementation step after the affine transmission path was moved onto:

- `TransmissionAnalysis` for cached structure
- `make_affine_plan_expected(...)` for affine planning
- `AffineJointMap` for runtime affine execution

The remaining major gap in Stage 2 is the grouped transmission runtime path.

## Current State

The codebase now has:

- cached grouped transmission structure in `TransmissionAnalysis`
- cached affine transmission structure in `TransmissionAnalysis`
- grouped planning over cached grouped transmissions in indexed space
- a working affine planner and affine runtime compilation path
- grouped compiler validation for:
  - stage topology
  - stage data availability
  - unsupported model build directions/quantities
- grouped ambiguity handling across multiple candidate plans
- builder-level grouped orchestration through `DefaultJointMapBuilder`
- builder-level grouped execution and copy-semantics test coverage

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
- affine transmission and mimic handling stay out of this path
- runtime execution should not allocate
- runtime execution should not do string lookup
- `DefaultJointMapBuilder` should not become the design center for this work

## Status

This grouped runtime plan is now substantially complete.

The reusable grouped planner/compiler/runtime structures exist, and `DefaultJointMapBuilder` is now acting as a thin
consumer of those structures rather than owning grouped semantics.

## Next Stage

The next work should come from the broader Stage 2 / transmission-spec documents rather than from this focused grouped
runtime note.

The most important remaining items are:

1. make quantity-specific transmission build behavior real
2. add the first real lightweight transmission compute/model implementation beyond test doubles
3. add plugin-specific builder extension coverage using the shared cached analysis
