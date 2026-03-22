# JointMap Stage 2 Incremental Plan

## Purpose

This note records the next incremental implementation step after the Stage 2 spec simplification.

The simplified direction is:

- `TransmissionModel` should describe runtime compute capability
- `TransmissionAnalysis` should own transmission topology explicitly
- `TransmissionAnalysis` should also own affine transmission relationships derived from mimic joints
- names should only exist at the analysis boundary, then be converted immediately to `JointId`

That means the next implementation step is not to add planning yet.
It is to finish the analysis-layer API shift so the planner is built on the correct shapes.

## Next Incremental Step

The next step is:

1. extract ros2_control transmission import out of `DefaultJointMapBuilder` into free helper functions
2. add affine transmission analysis records to `TransmissionAnalysis` for mimic-derived relationships
3. normalize mimic chains during ingestion so analysis stores one reduced affine relationship per mimic joint
4. ensure affine transmission and mimic records are not modeled through `TransmissionModel`
5. update planning work so affine transmission relationships remain eligible for direct `AffineJointMap` compilation

## Expected Result

After this step:

- `TransmissionAnalysis` should contain:
  - canonical joint ids
  - transmission models
  - transmission instances
  - affine transmission relationships derived from mimics
- grouped transmission instances should remain separate from affine transmission analysis records
- ros2_control import should no longer rely on builder-local parsing logic
- mimic handling should no longer be builder-private affine rewrite state
- the codebase should be ready for planner work that can select either affine fast-path compilation or grouped transmission compute

## Not Part Of This Step

This step does not yet:

- collapse grouped transmission execution into the affine fast path
- represent mimics through `TransmissionModel`
- implement the full grouped transmission runtime compute path

Those belong in the following steps once the analysis-layer shapes are stable.
