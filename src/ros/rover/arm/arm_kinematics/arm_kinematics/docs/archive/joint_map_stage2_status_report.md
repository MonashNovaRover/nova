## JointMap Stage 2 Status Report

### Purpose

This note summarizes the current state of Stage 2 after the transmission-analysis refactor, grouped runtime work,
ros2_control plugin-wrapper work, float-boundary migration, and the first mixed staged planning work.

It also records the main open questions before Stage 2 can be considered complete.

## Current Design State

The current design now follows the intended Stage 2 direction much more closely:

- `TransmissionAnalysis` is the cached structural owner of:
  - canonical joint ids
  - grouped `TransmissionInstance` records
  - affine transmission records derived from mimic and other affine-only relationships
- `ForwardKinematicsPlugin` is the authoritative seam for which `TransmissionAnalysis` is used by builders
- `RobotModel` provides a lazy shared default `TransmissionAnalysis`, not the final authority on analysis ownership
- the `joint_map` runtime boundary is now `float`-native, including the FK and collision seams that feed it
- ros2_control support now goes through the real transmission plugin system rather than hard-coded concrete
  transmission implementations inside `arm_kinematics`

The package now has reusable underlying structures for:

- affine planning and compilation
- grouped transmission planning and compilation
- mixed `JointMapPlan` planning
- runtime execution of:
  - `AffineJointMap`
  - grouped `TransmissionJointMap`
  - mixed `CompositeJointMap`
  - staged mixed `StagedJointMap`

## Work Completed

### 1. Analysis Ownership And Boundaries

- `TransmissionAnalysis` is indexed and canonical after the named boundary
- string lookup is confined to the description boundary
- mimic relationships are normalized into affine transmission analysis rather than builder-local mimic maps
- `TransmissionAnalysis` is copyable for FK-plugin-local reuse and augmentation

### 2. Runtime Scalar Boundary

- `JointMap`-layer runtime APIs now use `float`
- FK and collision seams that feed `JointMap` now also use `float`
- implicit `double`-to-`float` conversion inside the `joint_map` layer has been removed

### 3. Grouped Transmission Runtime

- grouped transmission planning exists in indexed space
- grouped transmission compilation validates:
  - topology
  - stage ordering / data availability
  - `TransmissionModel::can_build(...)`
- grouped runtime execution is allocation-free during `map(...)`
- reusable workspace is owned by the runtime map object
- multi-stage grouped execution through staged value buffers is supported

### 4. ros2_control Plugin Support

- ros2_control integration now uses the real `transmission_interface` loader plugin system
- a reusable ros2_control transmission plugin loader exists
- `RobotModel` provides a shared default loader for reuse
- `arm_kinematics` now wraps ros2_control transmission plugins generically instead of implementing per-type
  transmission math internally

### 5. Mixed Planning

The top-level `JointMapPlan` now supports useful mixed staged cases, including:

- parallel affine + grouped within one stage
- grouped -> affine
- affine -> grouped
- affine -> grouped -> affine
- grouped -> affine -> grouped

This is enough to demonstrate the intended direction:

- `JointMapPlan` is the top-level staged representation
- affine execution remains grouped into `AffineJointMap` segments
- grouped non-affine execution remains separate
- mixed execution can already compose those two forms

### 6. Typed Planner Errors

The planner has started moving away from stringly-typed failure handling:

- `make_joint_map_plan_expected(...)` now returns `JointMapPlanError`
  - `NoPlan`
  - `Ambiguous`
  - `Invalid`
- `make_transmission_plan_expected(...)` now returns `TransmissionPlanError`
  - `NoPlan`
  - `Ambiguous`
  - `Invalid`

This removed the earlier planner string-search seam and makes recursive planning behavior more structurally correct.

## Current Known Good Properties

The current implementation preserves the most important intended properties:

- setup-time analysis, runtime execution
- no string lookup in runtime mapping
- no heap allocation in real-time mapping paths
- affine relationships are not routed through `TransmissionModel`
- grouped non-affine transmission execution remains behind `TransmissionModel` / `ComputeTransmission`
- many affine relationships can compile into one `AffineJointMap` execution segment
- FK plugins can override or augment the shared default analysis without changing the runtime `JointMap` API

## Main Remaining Stage 2 Gap

The remaining major Stage 2 gap is not basic runtime support anymore.
It is planner policy and equivalence for broader mixed staged search.

Today, the planner has an explicit ambiguity/coalescing rule for the staged grouped-prefix versus staged
affine-prefix branch:

- if they are structurally equivalent, prefer the cheaper compute structure
- otherwise fail as ambiguous

That is directionally correct, but it is still narrower than the full Stage 2 design needs.

## Why Stage 2 Is Not Yet Complete

The current top-level mixed planner still lacks a principled cross-family equivalence rule for all competing staged
decompositions.

The concrete problem is:

- raw `JointMapPlan` shape is not enough to define semantic equivalence
- two plans can be compute-equivalent while having different staged shapes
- for example, a direct single-stage mixed plan and a staged tail decomposition may represent the same actual runtime
  work but appear structurally different in raw staged form

Without a better equivalence model, broadening ambiguity detection further would be risky:

- it could wrongly reject equivalent plans as ambiguous
- or worse, it could collapse distinct staged interpretations that should remain ambiguous under the spec

## Follow-Up Questions

These are the main questions that still need design answers.

### 1. What Should Count As The Same Mixed Plan?

We need a stronger definition of equivalence than raw `JointMapPlan` shape.

Questions:

- should equivalence be based on canonicalized execution segments rather than raw stages?
- should identity-carry segments be normalized away before comparison?
- should a direct single-stage mixed plan and a staged-tail plan be considered equivalent if they compile to the same
  effective affine/grouped execution structure?

### 2. Where Should Canonicalization Live?

If canonical execution signatures are the answer, we need to decide where they belong.

Questions:

- should `JointMapPlan` gain a normalization helper beside planning code?
- should canonicalization be a free helper used only for ambiguity resolution?
- should canonicalization happen only at comparison time, or should the planner emit canonicalized plans directly?

### 3. How Far Should Stage 2 Go On Mixed Search?

We should not overbuild Stage 2.

Questions:

- is Stage 2 complete once we have a principled equivalence rule and broader mixed ambiguity handling?
- or should Stage 2 also include broader automatic partitioning into maximal affine segments before we call it done?

### 4. What Builder Boundary Should Remain Public?

The reusable lower-level structures are now in much better shape.

Questions:

- should more callers be encouraged to use `TransmissionAnalysisJointMapBuilder` directly?
- or should `DefaultJointMapBuilder` remain the main visible entry point while staying a thin consumer?

### 5. What ros2_control Failure Coverage Still Matters In Stage 2?

The generic plugin-wrapper path is in place.

Questions:

- are there more ros2_control loader/build failure cases that should be covered in Stage 2?
- or is that now better treated as Stage 2.1 follow-up hardening rather than core Stage 2 work?

## Recommended Next Step

The next implementation step should be:

- define a canonical mixed-plan execution signature for ambiguity resolution
- use that signature first in the specific competing families already known to arise
- then broaden ambiguity handling beyond the current grouped-prefix versus affine-prefix branch

That is the smallest next step that moves the design forward without locking in the wrong interpretation of
mixed-plan equivalence.
