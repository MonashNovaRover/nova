# JointMap Stage 2 Progress Report

## Purpose

This document records the current implementation state of the Stage 2 `JointMap` transmission work, and the remaining steps required to finish it properly.

It is intentionally implementation-focused.
The Stage 2 spec remains the architectural source of truth.
This report answers:

- what exists in code today
- what is still scaffolding
- where the implementation has not yet converged with the intended design
- what work should happen next

## Current State

The codebase now contains the first real implementation slice of the Stage 2 design.

Implemented:

- Stage 2 transmission-side core types now exist:
  - `JointQuantity`
  - `PropagationDirection`
  - `TransmissionDefinition<TJoint>`
  - `NamedTransmissionDefinition`
  - `IndexedTransmissionDefinition`
- Transmission-side interfaces now exist:
  - `TransmissionModel`
  - `ComputeTransmission`
- A reusable `TransmissionAnalysis` type now exists.
- `TransmissionAnalysis` now owns transmission models and normalized group records.
- `DefaultJointMapBuilder` now provides `build_expected(..., JointQuantity)`.
- `JointMapBuilder::build(...)` is now a compatibility wrapper over `build_expected(...)`.
- `RobotModel` now exposes cached transmission analysis through `get_transmission_analysis()`.
- Parsed ros2_control transmission metadata is now normalized into owned transmission-side model records inside `TransmissionAnalysis`.
- `Order<>` has been refactored so its lookup storage is selected by key type:
  - contiguous numeric or enum keys use `std::vector`
  - `std::string` keys use `std::unordered_map`
  - other keys fall back to `std::map`
- Tests were added for:
  - transmission-analysis caching through `RobotModel`
  - normalized transmission group content
  - build-time reporting for recognized but unimplemented transmission-backed requests

Still scaffolding only:

- No real transmission-backed runtime `JointMap` exists yet.
- `ComputeTransmission` is still only an interface.
- No concrete transmission compute implementations exist.
- `TransmissionModel::build(...)` is still stubbed for the ros2_control-backed model path.
- `DefaultJointMapBuilder::build_expected(...)` still only succeeds for affine reorder or mimic requests.
- There is still no real structural transmission planning pass.

## What Is Working

The following behavior is now present and intentional:

1. Affine requests still work through the existing `AffineJointMap` fast path.
2. ros2_control transmission metadata is no longer just parsed and then forgotten.
   It is now converted into owned transmission-side models and grouped analysis data.
3. `RobotModel` is now the cache owner for transmission analysis, matching the broader "shared setup-time analysis" architecture used elsewhere in the package.
4. Transmission-backed requests now fail explicitly at build time rather than incorrectly falling through to affine-only behavior.
5. The builder API has started to move toward the intended Stage 2 shape by making quantity a build-time concern.
6. `Order<>` is now closer to supporting the intended analysis-boundary role for named lookups, although that integration work is not finished yet.

This is meaningful progress because it establishes:

- the ownership boundary
- the new type shapes
- the builder API evolution
- the first transmission-aware tests

## What Is Not Finished

The implementation is not yet at "Stage 2 complete."

### 1. No real structural transmission planning

`TransmissionAnalysis` currently stores:

- canonical joint ids
- transmission groups
- owned transmission models

But it does not yet support:

- planning from known input joints to required output joints
- selecting a valid propagation direction structurally
- rejecting ambiguity based on an actual plan search
- producing a real `TransmissionPlan`

Today, the builder only checks whether a failed affine request touches transmission-backed joints, and then reports that transmission mapping is recognized but not implemented yet.

### 2. No `TransmissionJointMap`

The Stage 2 design calls for a grouped runtime mapping type.
That runtime type does not exist yet.

Missing runtime pieces:

- `TransmissionJointMap`
- `CompiledTransmissionPlan`
- stage-local input/output index layouts
- per-stage scratch offsets and sizes
- preallocated scratch ownership at runtime
- actual grouped stage execution

### 3. No concrete `ComputeTransmission`

`ComputeTransmission` currently defines the runtime interface only.

Still needed:

- at least one concrete compute implementation
- a simple first transmission-backed example used in tests
- integration between `TransmissionModel::build(...)` and concrete compute objects

### 4. No real quantity-specific transmission build logic

The current design says quantity should matter only while building the map.

The code does not yet implement:

- meaningful `can_build(...)` behavior for real transmission models
- quantity-specific build selection
- distinct position and velocity build products where required

### 5. The `Order<>` integration decision is not finished

`Order<>` itself has moved forward, but the transmission side has not converged on exactly how to use it.

At the moment:

- `TransmissionAnalysis` still uses explicit name/id storage internally
- the Stage 2 spec still discusses a canonical analysis-boundary `Order<>`
- the code does not yet use `Order<>` as the transmission-analysis boundary representation

That is acceptable temporarily, but it is still an open design-to-code convergence point.

### 6. The current ros2_control model is structural only

The current ros2_control-backed transmission model:

- exposes a normalized definition
- contributes groups into analysis
- always returns `false` from `can_build(...)`
- throws from `build(...)`

So it is currently an analysis/import scaffold, not yet a real build-capable transmission model.

## Current Risks And Incomplete Areas

### 1. No compile-backed verification in this environment

This environment still does not have the ROS `ament_cmake` toolchain available, so the current Stage 2 changes have not been compile-verified here.

That means the main immediate engineering risk is still:

- template fallout from the `Order<>` refactor
- interface fallout from the new transmission-side types
- ordinary integration errors that only the compiler will surface

### 2. `Order<>` is now deeper infrastructure

The recent `Order<>` changes are a real internal refactor, not just a doc or naming change.

That makes it more important to verify:

- construction behavior
- inverse behavior
- composition behavior
- constrained APIs for contiguous-key-only operations

before relying on it more heavily in transmission planning.

### 3. `JointMap` runtime copy semantics may matter later

`JointMap` currently remains a value wrapper with cloning semantics.
When transmission-backed runtime maps are added, the implementation will need to preserve that contract cleanly.

That likely means one of:

- transmission runtime maps remain deeply cloneable
- transmission compute stages provide their own cloning support

This is not a blocker yet, but it should be kept in mind while introducing `TransmissionJointMap` and compiled stage storage.

## Recommended Next Steps

The remaining work to finish Stage 2 properly should be:

### Step 1. Build and stabilize the current branch

Before extending the implementation further:

- build the package in a normal ROS environment
- fix compiler fallout from the new transmission-side interfaces
- fix compiler fallout from the `Order<>` storage refactor
- run the existing tests and repair any breakage

This is the highest-priority next step because the current branch has not been compile-verified here.

### Step 2. Finalize the analysis-boundary representation

Decide explicitly whether `TransmissionAnalysis` should:

- use `Order<>` as its canonical named boundary mapping
- keep explicit name/id storage
- or wrap one inside the other

Then update both code and spec so they match.

The important outcome is not forcing `Order<>` into the design everywhere.
The important outcome is having one clear, consistent boundary representation for named joint ids.

### Step 3. Add structural transmission plan types

Introduce:

- `TransmissionPlanStage`
- `TransmissionPlan`

These should remain structural only:

- grouped `JointId` sets
- chosen `PropagationDirection`
- no runtime quantity tags

### Step 4. Implement the first real planner

Move `DefaultJointMapBuilder::build_expected(...)` from "recognize and fail" to real planning.

The first planner should at least handle:

- affine-only requests
- direct single-group transmission requests
- forward and reverse group selection
- precise build-time failure for unsupported or ambiguous requests

It does not need to solve globally optimal multi-stage planning yet.
It does need to be structurally correct and deterministic.

### Step 5. Introduce runtime transmission execution

Add:

- `CompiledTransmissionPlan`
- `TransmissionJointMap`
- preallocated scratch layout and ownership

This should be the first real transmission-backed runtime path.

### Step 6. Implement the first concrete `ComputeTransmission`

Start with one real transmission-backed compute implementation.

The best first target is the smallest case that exercises the full pipeline:

- builder selects a transmission-capable model
- model builds a compute object
- runtime map executes grouped math
- scratch is passed but not allocated in the hot loop

Coverage breadth matters less than proving the architecture end-to-end.

### Step 7. Make quantity-specific build behavior real

Once there is a real planner and a real runtime transmission path:

- make `can_build(...)` meaningful
- build quantity-specific compute objects where needed
- add tests showing that quantity selection is purely a build-time decision

### Step 8. Expand tests around planning and runtime behavior

Add tests for:

- direct plan construction
- forward vs reverse selection
- ambiguous request failure
- unsupported quantity failure
- runtime transmission execution correctness
- scratch-buffer reuse and allocation-free execution assumptions
- any `Order<>` behavior now relied on by transmission analysis or planning

## Suggested Sequencing

Recommended order:

1. Compile and test the current branch in a ROS environment.
2. Finalize the `Order<>` / transmission-boundary representation decision.
3. Add structural `TransmissionPlan` types.
4. Implement the first real planner.
5. Add `CompiledTransmissionPlan` and `TransmissionJointMap`.
6. Add the first concrete `ComputeTransmission`.
7. Make quantity-specific build logic real.
8. Expand tests and tighten docs to match the final implementation.

## Assessment

The implementation is at a useful midpoint:

- the architecture is no longer only in planning documents
- transmission ownership and caching are now in code
- the builder API has begun moving toward the intended Stage 2 shape
- the affine path is still intact

But the transmission path is still scaffolded, not finished functionality.

The honest status is:

- Stage 2 scaffolding: present and meaningful
- Stage 2 runtime functionality: not finished
- best next move: compile and stabilize the current branch, then implement real planning and runtime transmission execution
