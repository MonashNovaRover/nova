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

What is still missing is the grouped runtime execution side:

- no integration yet back into `DefaultJointMapBuilder`
- no builder-facing consumption of the completed grouped planner/compiler/runtime path

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

## Design Constraints

- `TransmissionModel` remains the build-time capability interface only
- `ComputeTransmission` remains the runtime grouped compute interface
- affine transmission and mimic handling stay out of this path
- runtime execution should not allocate
- runtime execution should not do string lookup
- `DefaultJointMapBuilder` should not become the design center for this work

## Next Implementation Step

The next incremental step is to consume the completed grouped planner/compiler/runtime path from the standard builder
without letting the builder become a semantic owner.

That step should:

1. re-enable grouped orchestration in `DefaultJointMapBuilder`
2. keep the builder limited to:
   - boundary name conversion
   - affine path selection
   - grouped path selection
3. make the builder delegate to the grouped planner/compiler/runtime structures without duplicating grouped logic
4. add end-to-end tests proving builder-produced grouped joint maps execute correctly

## Recommended Order

1. re-enable grouped path selection in `DefaultJointMapBuilder`
2. keep grouped planning/compilation in the existing free/helper structures
3. add builder-level grouped execution tests
4. only after that, consider planner refinements or performance cleanup if needed
