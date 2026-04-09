## JointMap Stage 2 Part 2: Algorithm Report

### Purpose

This document records a stronger algorithm direction for Stage 2 mixed planning.

It is intentionally written from first principles rather than from the current implementation shape.
The goal is to establish:

- a deterministic planning model
- strong invariants
- a canonical notion of planner state
- a principled ambiguity rule

This note should be used to guide future implementation refactors, not to justify the current implementation by
backfilling explanations.

## Problem Statement

Given:

- a caller input joint set
- a caller requested output joint set
- a cached `TransmissionAnalysis` containing:
  - affine transmission relationships
  - grouped non-affine `TransmissionInstance` relationships

We need to determine one of:

1. a valid runtime `JointMap` execution plan
2. proof that no plan exists
3. proof that the request is ambiguous

The plan must be:

- indexed
- deterministic
- correct with respect to `TransmissionAnalysis`
- compatible with compilation into runtime `JointMap` execution structures

## Design Goal

The planner should not be thought of as:

- “find a source for each output”
- or “recursively try mixed cases until something works”

It should be thought of as:

- planning over reachable joint spaces
- under deterministic affine saturation
- with grouped non-affine transitions as the only true branch points

## Core Idea

The strongest high-level idea is:

`Affine closure is deterministic. Grouped transitions are the real search steps.`

That means:

- affine propagation should not be treated as a search branch
- grouped transmission choices should be the only places where the planner branches
- every search state should already be affine-saturated

This is the key step that should keep the planner deterministic and avoid accidental plan-shape drift.

## Proposed Core Invariants

### 1. Planning is over joint spaces, not individual outputs

The planner state is never “how do I compute output X?”

It is:

- what canonical joint space is currently available
- what grouped transitions are legal from that space
- whether the requested outputs are already contained in that space

### 2. Affine closure is semantic and mandatory

For any currently available joint set, there is one maximal affine closure implied by `TransmissionAnalysis`.

The planner must:

- compute that closure deterministically
- treat that closure as the semantic current state
- never branch on partial application of affine relationships

This means:

- many affine transmissions should collapse together automatically
- a request-local affine segment should be maximal by construction unless a non-affine transition forces a boundary

### 3. Grouped transitions are the only true branch points

Planner branching should occur only when choosing which grouped `TransmissionInstance` can be applied next from the
current affine-closed state.

Affine propagation is not a branch.

### 4. Search states must be canonical

Two states that represent the same semantically available joint space should compare equal unless some additional
execution provenance is truly required by the final semantics.

At minimum, a planner state should not differ merely because:

- affine relationships were discovered in a different order
- equivalent staging artifacts were introduced during search

### 5. Stage boundaries are semantic, not syntactic

A stage boundary should only exist when required by execution semantics.

Conceptually:

- start from an affine-closed space
- apply one grouped transition wave
- then recompute affine closure

The planner should not invent alternate stage layouts for the same semantic execution.

### 6. Ambiguity is defined on canonical execution meaning

Two valid plans are ambiguous only if, after canonicalization, they still represent distinct valid execution
structures.

If canonicalization makes them equivalent, they are not ambiguous.

### 7. Cost is only a tiebreak after equivalence

Cost may only be used after canonical equivalence is established.

It must never be used to choose between semantically distinct plans.

### 8. Planner output should already reflect runtime structure

The final planner output should be close to runtime execution form.

That means it should naturally express:

- maximal affine execution segments
- grouped non-affine segments between them
- explicit dependency ordering

The planner should not produce an arbitrary proof artifact that later needs a second semantic restructuring pass.

## Suggested Conceptual State Model

The strongest candidate state model is:

- current affine-closed available joint set
- canonical execution prefix

Potentially also:

- grouped transmission instances already used, if reuse must be restricted

The important part is that the current state should already be affine-saturated.

That removes a large amount of accidental branching.

## Suggested Transition Model

### Step 1: Start from caller input joints

Take the caller input joint ids as the initial available space.

### Step 2: Compute maximal affine closure

Compute the full affine closure from that input space.

This is deterministic.
This becomes the first semantic state.

### Step 3: Check for completion

If the requested outputs are contained in the current affine-closed state:

- planning succeeds
- the remaining work is to materialize the canonical execution prefix plus the final affine segment

### Step 4: Enumerate legal grouped transitions

From the current affine-closed state, enumerate grouped `TransmissionInstance` transitions that:

- can build for the requested `JointQuantity`
- have all consumed joints available in the current state
- add new information to the reachable joint space

These are the legal search branches.

### Step 5: Apply grouped transition, then immediately re-saturate

For each chosen grouped transition:

- apply the grouped transition structurally
- extend the available joint set
- immediately recompute maximal affine closure

The resulting affine-closed space is the next canonical state.

### Step 6: Continue until solved, impossible, or ambiguous

The planner continues until:

- outputs are satisfied
- no legal grouped transition remains
- or multiple canonically distinct valid plans are found

## Grouped Transition Waves

One strong conceptual refinement is to treat grouped transitions as waves.

A wave would mean:

- all grouped transitions in that wave consume only from the current affine-closed state
- grouped transitions in the same wave do not consume each other’s outputs
- all outputs of the wave become available together
- affine closure is then recomputed after the wave completes

This may be a better semantic unit than arbitrary stage segmentation.

Advantages:

- stronger determinism
- easier conflict reasoning
- easier canonicalization
- cleaner separation between affine saturation and grouped execution

However, this is still a design question rather than a settled requirement.

## Open Algorithm Questions

### 1. Should grouped transitions be planned one-at-a-time or in waves?

One-at-a-time planning is simpler.
Wave-based planning may be more canonical and more aligned with eventual runtime segmentation.

This needs an explicit decision.

### 2. Can grouped transitions in the same wave consume each other’s outputs?

My current inclination is no.

They should consume only from the current affine-closed state.
Then outputs become visible after the wave completes and affine closure is recomputed.

That keeps waves semantically clean.

### 3. Can a `TransmissionInstance` be used more than once in one plan?

This should be decided explicitly.

The answer affects:

- planner state identity
- loop prevention
- correctness of grouped search

### 4. What should count as conflict between grouped transitions?

Potential conflicts could include:

- overlapping produced joints
- incompatible consumed/produced relationships
- semantically distinct ways to derive the same later reachable state

This needs a precise rule, not an ad hoc one.

### 5. What is the canonical execution signature?

This is the main unresolved design question.

We need a canonical representation strong enough to distinguish:

- genuinely conflicting valid plans
- semantically equivalent plans with different incidental staging shape

This signature is likely the key to correct ambiguity handling.

## Why This Is Better Than Continuing Incrementally

The current implementation has already shown the main risk:

- if the planner grows branch-by-branch, ambiguity policy becomes accidental
- if equivalence is defined by raw staged shape, equivalent plans can look different
- if cost is applied before canonical equivalence, the planner can silently make semantic choices

A stronger algorithm spec avoids those problems by fixing:

- the search state
- the transition rules
- the saturation model
- the ambiguity rule

before more search behavior is added.

## Recommended Next Non-Code Step

Before changing more implementation, the next design work should answer:

1. whether grouped planning should be wave-based
2. whether grouped transitions may consume outputs from the same wave
3. whether transmission instances may repeat in one plan
4. what the canonical execution signature should be

Only after those are settled should the mixed planner be refactored further.

## Summary

The strongest version of the Stage 2 planner appears to be:

- joint-space based
- affine-saturating
- grouped-transition branching
- canonically stateful
- ambiguity-aware only after canonical execution comparison

That is a much stronger foundation than continuing to extend the current mixed planner by local case analysis.
