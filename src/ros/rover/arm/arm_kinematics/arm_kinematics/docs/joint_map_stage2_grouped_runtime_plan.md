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

- no planner yet beyond the current direct and simple two-stage grouped search
- no general multi-stage grouped search over `TransmissionAnalysis`
- no integration yet back into `DefaultJointMapBuilder`

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

The next incremental step is to finish grouped compiler correctness and planner policy before broadening grouped
search further.

That step should:

1. make `compile_transmission_plan_expected(...)` reject unsupported stage builds explicitly
2. return grouped compiler failures through `tl::expected` rather than relying on runtime exceptions
3. add tests for unsupported grouped-stage compilation failures
4. decide and document ambiguity policy when both direct and multi-stage grouped candidates exist
5. only after that, broaden grouped planning beyond the current direct and simple two-stage cases

## Recommended Order

1. finish grouped compiler failure semantics
2. add tests for unsupported build rejection
3. settle grouped ambiguity policy across direct and multi-stage candidates
4. then broaden the planner from exact direct match/simple two-stage search to more general indexed grouped search
5. only after that, adapt `DefaultJointMapBuilder` to consume the completed grouped path
