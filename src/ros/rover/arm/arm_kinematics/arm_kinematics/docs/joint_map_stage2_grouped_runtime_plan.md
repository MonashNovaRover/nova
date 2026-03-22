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

- no compiled grouped stage representation
- no compiled grouped plan representation
- no grouped runtime `JointMap` implementation
- no adapter from `TransmissionPlan` to runtime compute stages

## Next Implementation Step

The next step is to implement the first grouped runtime execution structures independent of `DefaultJointMapBuilder`.

That step should:

1. define a compiled grouped stage type around `ComputeTransmission`
2. define a compiled grouped plan type with:
   - input count
   - output count
   - scratch size
   - compiled stages
3. define a grouped runtime map type that executes the compiled grouped plan
4. keep all of this indexed and preallocated
5. avoid introducing names or `JointQuantity` into runtime execution

## Design Constraints

- `TransmissionModel` remains the build-time capability interface only
- `ComputeTransmission` remains the runtime grouped compute interface
- affine transmission and mimic handling stay out of this path
- runtime execution should not allocate
- runtime execution should not do string lookup
- `DefaultJointMapBuilder` should not become the design center for this work

## Recommended Order

1. add `CompiledTransmissionStage`
2. add `CompiledTransmissionPlan`
3. add `TransmissionJointMap`
4. add a first compiler helper from `TransmissionPlan` to `CompiledTransmissionPlan`
5. keep the compiler limited to the current direct single-stage grouped plan case
6. add tests around grouped plan compilation and runtime execution
7. only later integrate that into the builder
