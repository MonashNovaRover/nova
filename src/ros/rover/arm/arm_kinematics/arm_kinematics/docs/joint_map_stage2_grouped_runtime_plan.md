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
- a direct grouped transmission planner for the simplest one-stage exact-match case
- a working affine planner and affine runtime compilation path

What is still missing is the grouped runtime execution side:

- no validation yet that each compiled stage respects the cached transmission topology
- no validation yet that multi-stage grouped plans are topologically executable
- no planner yet for multi-stage grouped plans

## Current Runtime Scope

The codebase now has:

1. `CompiledTransmissionStage`
2. `CompiledTransmissionPlan`
3. `TransmissionJointMap`
4. a compiler from `TransmissionPlan` to `CompiledTransmissionPlan`
5. scratch sizing via `ComputeTransmission::scratch_size()`

This runtime path now supports:

- direct single-stage grouped plans from `make_transmission_plan_expected(...)`
- manually constructed multi-stage grouped plans at compile/runtime level
- stage-to-stage dataflow through a compiled value buffer

What it still does not support is deriving those multi-stage plans automatically from `TransmissionAnalysis`.

## Design Constraints

- `TransmissionModel` remains the build-time capability interface only
- `ComputeTransmission` remains the runtime grouped compute interface
- affine transmission and mimic handling stay out of this path
- runtime execution should not allocate
- runtime execution should not do string lookup
- `DefaultJointMapBuilder` should not become the design center for this work

## Next Implementation Step

The next incremental step is to tighten grouped plan compilation correctness before broadening grouped planning.

That step should:

1. validate that each `TransmissionPlanStage` matches the referenced cached transmission topology
2. validate that each stage consumes only:
   - plan inputs
   - or values produced by earlier stages
3. reject grouped plans that rely on future-stage outputs or unrelated joint ids
4. add tests for invalid grouped plans
5. only after that, broaden `make_transmission_plan_expected(...)` toward multi-stage grouped search

## Recommended Order

1. validate stage topology against `TransmissionAnalysis::transmissions()`
2. validate stage ordering/data availability in `compile_transmission_plan_expected(...)`
3. add failure tests for malformed grouped plans
4. then broaden the planner toward real multi-stage grouped plan construction
5. keep `DefaultJointMapBuilder` out of that work until the grouped path is structurally complete
