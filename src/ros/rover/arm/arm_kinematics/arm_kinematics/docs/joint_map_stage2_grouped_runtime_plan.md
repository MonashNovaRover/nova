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

- no validation yet for nonzero scratch layout
- no proof yet that runtime scratch is reused correctly across calls
- no support yet for multi-stage grouped plans
- no support yet for stage-to-stage dataflow beyond the current direct single-stage case

## Current Runtime Scope

The codebase now has:

1. `CompiledTransmissionStage`
2. `CompiledTransmissionPlan`
3. `TransmissionJointMap`
4. a compiler from `TransmissionPlan` to `CompiledTransmissionPlan`
5. scratch sizing via `ComputeTransmission::scratch_size()`

This runtime path is intentionally limited to the current direct single-stage grouped plan case.

## Design Constraints

- `TransmissionModel` remains the build-time capability interface only
- `ComputeTransmission` remains the runtime grouped compute interface
- affine transmission and mimic handling stay out of this path
- runtime execution should not allocate
- runtime execution should not do string lookup
- `DefaultJointMapBuilder` should not become the design center for this work

## Next Implementation Step

The next incremental step is to validate the grouped runtime contract before broadening planner scope.

That step should:

1. add a grouped runtime test with nonzero scratch
2. verify `compile_transmission_plan_expected(...)` assigns:
   - per-stage `scratch_offset`
   - per-stage `scratch_size`
   - total `CompiledTransmissionPlan::scratch_size`
3. verify `TransmissionJointMap` reuses the owned scratch workspace across repeated calls
4. keep the implementation limited to the current direct single-stage grouped case

## Recommended Order

1. add a test `ComputeTransmission` with nonzero scratch requirements
2. assert compiled scratch layout in tests
3. assert runtime execution correctness using scratch-backed compute
4. only after that, broaden the planner/compiler toward multi-stage grouped execution
5. keep `DefaultJointMapBuilder` out of that work until the grouped path is structurally complete
