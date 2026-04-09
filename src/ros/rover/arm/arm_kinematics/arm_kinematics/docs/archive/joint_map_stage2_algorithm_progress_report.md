## JointMap Stage 2 Algorithm Progress Report

### Purpose

This report summarizes the current conceptual progress on the Stage 2 planning algorithm.

It is focused on the algorithm and proof model, not on the current implementation shape.
The intent is to capture the stronger semantic direction that should guide any future refactor.

## Current Direction

The current design direction is now:

- define a proof-level semantic dependency model first
- prove correctness and non-ambiguity on that model
- only afterward derive runtime staging and optimization rules

This is explicitly stronger than continuing to extend the current mixed planner incrementally.

## Main Shift In Perspective

The important shift is:

- do not start from staged `JointMapPlan` execution structure
- start from the unique semantic dependency graph implied by the request

That means:

- correctness is first a graph-theoretic question
- staging is derived later
- runtime optimization is derived after correctness, not mixed into correctness

## Proof Model Under Discussion

The current leading proof model is a bipartite DAG-like dependency graph containing:

- source joint nodes for input joints
- sink joint nodes for requested output joints
- affine-closed-group / affine-chain nodes representing deterministic affine propagation structure
- transmission-instance nodes
- unique transmission-input attachment nodes
- unique transmission-output attachment nodes

Conceptually:

1. each input joint connects into the affine structure it belongs to
2. each requested output is read from the affine structure that defines it
3. each grouped transmission instance:
   - consumes through unique input attachment nodes
   - produces through unique output attachment nodes
   - connects to affine structure on both sides

This graph is intended as a proof object, not necessarily as the final runtime planning data structure.

## Main Invariants Agreed So Far

### 1. Minimal correctness model first

The first algorithm should be the simplest model that is strong enough to prove correctness.

Fast paths and simplifications should come later.

### 2. Transmission instances are single-use

An instance may appear at most once in a valid plan.

Reason:

- an instance is not a reusable model template
- it represents one structural transformation in the problem
- if the same instance appears twice in a plan, the plan is redundant

### 3. Dependency ordering must be respected

If any output of grouped transmission `A` is reachable from any input of grouped transmission `B`,
then `A` must execute before `B`.

This is stronger and more minimal than the earlier “wave” model.

### 4. Wave semantics are not the minimal basis

The earlier wave abstraction is now viewed as too coarse for the first correctness model.

It may still be useful later as a derived execution grouping, but it should not be the foundational semantic object.

### 5. Canonical semantic structure matters more than current planner branch shape

We should not derive the algorithm from the current recursive mixed planner.

The algorithm should be derived from:

- graph semantics
- unique definition rules
- dependency constraints
- ambiguity rules

## Semantic Graph Validity Conditions

The current strong idea is:

if the induced dependency graph is valid and non-ambiguous, then:

- every in-degree-0 node must be a source
- every sink must be uniquely defined
- no source may gain an incoming defining edge
- no sink may have zero or multiple conflicting definitions

This leads to the useful separation:

- `NoPlan` / undefined:
  - some required value is not derivable
- `Ambiguous`:
  - some required value has multiple conflicting derivations
- valid:
  - every required value is uniquely derivable

## Theorem Direction

The current theorem direction is:

If the induced semantic dependency graph is acyclic and every required dependency site is uniquely defined,
then the request has a unique semantic dependency structure.

That theorem is intended to establish:

- meaning first
- scheduling later

The theorem does not yet try to prove optimal runtime staging.

## Depth / Layering Insight

Another important insight is that, once the semantic DAG is fixed and non-ambiguous:

- depth is well-defined
- dependency layering becomes deterministic

Earlier, the discussion considered assigning affine-stage index directly from affine-group depth.

That is now considered too simplistic for runtime purposes.

## Important Correction: Semantic Birth Layer Is Not Runtime Placement

The current refined view is:

- a semantic dependency DAG can tell us the earliest layer at which a value can exist
- but runtime compilation must decide when to actually materialize that value

So we now distinguish:

### 1. Semantic availability layer

The earliest layer where a value is derivable from the dependency graph.

### 2. Runtime materialization layer

The affine layer where we actually choose to compute/store the value for runtime execution.

This distinction matters because runtime scratch/buffer cost depends on value lifetime, not just on semantic birth.

## Current Runtime Optimization Insight

The current best optimization direction is:

- assign affine mapping layers to transmission-input values and sink values
- not directly to affine-closed-group nodes

The reason is that runtime cost is driven by lifetimes.

For each value that must be materialized:

- it has an earliest layer where it could exist
- it has one or more consumer layers where it is needed
- it should be materialized as late as possible while still satisfying all consumers

That minimizes lifetime in intermediate buffers.

## Current Optimization Objective

The current candidate runtime optimization objective is:

minimize the sum of all value lifetimes measured in layer count.

In other words:

- compute values as late as possible
- keep them live for as few layers as possible
- derive affine map placement from that

This is now considered a better runtime objective than assigning affine mapping index purely from node depth.

## Current Conceptual Separation

The emerging design separation is:

### Phase 1: Semantic correctness

- construct or reason about the induced dependency graph
- verify acyclicity
- verify unique definition of all required values
- determine whether the request is valid, ambiguous, or impossible

### Phase 2: Dependency-respecting ordering

- derive an execution order consistent with grouped transmission dependencies

### Phase 3: Runtime affine materialization optimization

- decide where values should actually be materialized
- minimize total lifetime cost
- derive affine execution segments from those assignments

This separation is much stronger than trying to discover all three concerns at once through recursive mixed planning.

## Open Questions

The main open questions now are:

1. What is the cleanest formal definition of the semantic proof graph?
   In particular:
   - exact node classes
   - exact edge classes
   - whether “affine-chain” or “affine-closed-group” is the better formal term

2. What exact theorem should define graph validity and uniqueness?

3. What is the correct minimal scheduling theorem once the semantic graph is fixed?

4. How should the runtime lifetime-minimization problem be formulated?
   Likely as:
   - latest-feasible materialization assignment
   - or equivalent liveness minimization across layers

5. What canonical runtime structure should be produced from that lifetime-minimized solution?

## Summary

The strongest current position is:

- the mixed planning problem should be grounded in a proof-level semantic dependency DAG
- correctness should be established there first
- grouped transmission instances are single-use
- dependency ordering should be derived from graph reachability, not from ad hoc planner branches
- runtime affine layer placement should be based on lifetime minimization for required values, not directly on
  affine-group depth

This is now a significantly stronger conceptual foundation than the earlier implementation-driven staged planning
approach.
