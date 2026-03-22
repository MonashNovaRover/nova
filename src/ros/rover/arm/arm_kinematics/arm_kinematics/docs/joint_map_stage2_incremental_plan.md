# JointMap Stage 2 Incremental Plan

## Purpose

This note records the next incremental implementation step after the Stage 2 spec simplification.

The simplified direction is:

- `TransmissionModel` should describe runtime compute capability
- `TransmissionAnalysis` should own transmission topology explicitly
- names should only exist at the analysis boundary, then be converted immediately to `JointId`

That means the next implementation step is not to add planning yet.
It is to finish the analysis-layer API shift so the planner is built on the correct shapes.

## Next Incremental Step

The next step is:

1. make `TransmissionAnalysis::add_model(...)` register only transmission models
2. make `TransmissionAnalysis::add_transmission(...)` register only transmission topology
3. ensure the named overload of `add_transmission(...)` converts names immediately through `joint_order()`
4. update ros2_control ingestion to use those two operations explicitly
5. update tests to assert `joint_order()` and `transmissions()` rather than the older mixed model/topology shape

## Expected Result

After this step:

- `TransmissionAnalysis` should contain:
  - canonical joint ids
  - transmission models
  - transmission instances
- transmission instances should be the only stored topology records
- ros2_control import should no longer rely on topology being queried back from `TransmissionModel`
- the codebase should be ready for the first real planner types without another boundary refactor

## Not Part Of This Step

This step does not yet:

- remove topology-related compatibility scaffolding from `TransmissionModel`
- add `TransmissionPlan`
- add `TransmissionJointMap`
- implement any real transmission runtime compute path

Those belong in the following steps once the analysis-layer shapes are stable.
