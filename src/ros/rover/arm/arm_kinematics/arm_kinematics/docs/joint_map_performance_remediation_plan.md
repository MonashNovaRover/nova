# Joint Map Builder — Performance Remediation Plan

**Date:** 2026-04-12
**Scope:** `TransmissionReachability`, `plan_joint_map`, `materialize_joint_map`, and the API boundaries that currently force avoidable work
**Primary input:** [`docs/performance-analysis.md`](./performance-analysis.md)

## Goal

Reduce builder cost by correcting implementation and API decisions that create unnecessary hashing,
allocation, repeated symbolic resolution, and rebuild work.

This plan is intentionally **not** a "micro-optimise everything" exercise. The focus is first on
removing mistakes that make the current code slower than it needs to be while preserving current
behaviour and correctness.

## Framing

The profile in [`docs/performance-analysis.md`](./performance-analysis.md) points at real local
costs:

- too many heap allocations
- too many hash-table lookups in dense-id code paths
- repeated lookup of `StateInterfaceDefinition` values that carry string-heavy equality

Those findings are valid. The wrong conclusion would be to treat every hot allocation as a reason
to add arenas or to weaken equality semantics.

The better conclusion is:

1. We are using the wrong representations in several inner loops.
2. Some of those representation mistakes are implementation details.
3. Some of them are API design mistakes, because current APIs force later stages to recover
   information that earlier stages already knew.

This plan therefore includes API changes when they remove structural inefficiency rather than
papering over it.

## Non-goals

- Do not weaken correctness by switching collision-safe equality to hash-only equality.
- Do not introduce custom allocators, arenas, or memory pools until representation mistakes are
  fixed and re-profiled.
- Do not redesign the entire joint-map pipeline around caching or precompilation in this first
  pass. Those may become appropriate later, but they should not hide avoidable mistakes in the
  current implementation.
- Do not optimise the runtime `JointMap::map` path based on builder profiling alone.

## Main Problems To Correct

### 1. Dense ids are treated like sparse keys

Several hot structures use `unordered_map<StateInterfaceId, ...>` or
`unordered_map<TransmissionInstanceId, ...>` even though those ids are dense, zero-based indices.

This appears in:

- [`src/joint_map/joint_map_blueprint.cpp`](../src/joint_map/joint_map_blueprint.cpp)
- [`src/joint_map/transmission_reachability.cpp`](../src/joint_map/transmission_reachability.cpp)

That choice adds:

- hashing work
- bucket traversal
- node allocation
- rehash churn

for lookups that should be O(1) indexed loads from a `vector`.

### 2. Symbolic keys survive too long

`StateInterfaceDefinition` is a good boundary type, but it is overused in internal build-stage
data structures. Once a definition has a canonical internal id, later stages should stop using the
symbolic form for lookups.

This is most visible in:

- [`src/joint_map/materialize_joint_map.cpp`](../src/joint_map/materialize_joint_map.cpp)
- [`src/joint_map/transmission_reachability.cpp`](../src/joint_map/transmission_reachability.cpp)

The current design forces repeated resolution of definitions that were already known earlier in the
pipeline.

### 3. The blueprint is too symbolic

`plan_joint_map()` computes enough information to know execution order and dependencies, but the
resulting `JointMapBlueprint` still leaves `materialize_joint_map()` to perform another round of
scratch-slot assignment and key resolution.

This is not just an implementation issue. It is an API design issue in the boundary between:

- `plan_joint_map()`
- `JointMapBlueprint`
- `materialize_joint_map()`

The current API shape causes repeated work by design.

### 4. Avoidable vector churn still exists even where vectors are already used

Examples:

- `ready.erase(ready.begin())` in the topological sort
- repeated `std::find`-based uniqueness construction
- missing `reserve()` in many structures whose size is knowable up front

These are secondary to the representation issues above, but they are still mistakes worth fixing.

## Design Principles For The Remediation

### Preserve correctness first

Do not trade correctness for speed in core identifiers.

In particular, keep `InterfaceId` collision-safe for public and diagnostic-facing semantics.
If string-backed identifiers are too expensive internally, the fix is to stop using them in hot
internal structures, not to pretend collisions cannot happen.

### Push canonicalisation earlier

Canonicalise as close to the boundary as possible, then keep later stages in canonical id space.

Practical meaning:

- public API may accept `StateInterfaceDefinition`
- internal builder stages should prefer compact plan-local indices and `TransmissionInstanceId`
- `StateInterfaceId` should be treated as an optional analysis-specific optimisation, not as the
  default representation the planner is built around
- definitions that are not registered in `TransmissionAnalysis` must not be treated as a special
  slow fallback if they are the common case

### Make later stages dumber

If the planner already knows scratch slots, gather indices, or output scatter positions, the
materialiser should receive that directly rather than recomputing it.

### Avoid a global `StateInterfaceId` bias

The current code conflates two different things:

- analysis-local ids that exist only when `TransmissionAnalysis` has registered a definition
- layer-local ids that exist to make one stage's internal storage compact
- the planner's need for a compact canonical handle for "state-like things" regardless of whether
  they were registered in the analysis

The remediation should not assume that "registered SID" is the normal case. If unregistered
definitions are common, then the planner needs its own local canonical ids rather than treating
non-SID definitions as a secondary path.

The boundary decisions made so far are consistent with this:

- `TransmissionAnalysis::StateInterfaceId` is analysis-local
- `TransmissionReachability::StateInterfaceId` is reachability-local
- `JointMapBlueprint` uses blueprint-local ids

Those aliases currently share the same underlying representation, but they should not be treated
as semantically interchangeable.

## Phase 1 — Remove Obvious Representation Mistakes

### Objective

Cut allocator and hash-table cost without changing user-visible behaviour or requiring a new public
API.

### Work items

1. Replace dense-id maps in `plan_joint_map()`.
   Files:
   - [`src/joint_map/joint_map_blueprint.cpp`](../src/joint_map/joint_map_blueprint.cpp)

   Changes:
   - Replace `dependents` with `std::vector<std::vector<TransmissionInstanceId>>` indexed by
     `TransmissionInstanceId`.
   - Replace `in_degree` with `std::vector<std::size_t>`.
   - Replace `stage_index_of` with `std::vector<std::optional<std::size_t>>` or a sentinel-based
     vector sized to `analysis.transmissions().size()`.

   Notes:
   - This assumes `TransmissionInstanceId` is dense and zero-based, which is already how the code
     behaves.
   - The representation should be sized from `analysis.transmissions().size()`, not from
     `required.size()`, to keep indexing simple and branch-free.

2. Replace dense-id maps in `TransmissionReachability` where the ids are truly dense.
   Files:
   - [`include/arm_kinematics/joint_map/transmission_reachability.hpp`](../include/arm_kinematics/joint_map/transmission_reachability.hpp)
   - [`src/joint_map/transmission_reachability.cpp`](../src/joint_map/transmission_reachability.cpp)

   Changes:
   - Replace `producer_assignment_` with storage that is vector-indexed only where the key space is
     genuinely dense for that reachability run.
   - Replace pass-local `candidates` and `ambiguity_snapshots` with vector-indexed storage only for
     those entries that have been canonicalised into a dense local index space.
   - Do not assume the correct split is "SIDs fast, bare defs slow". The split should instead be
     "canonicalised local ids fast, symbolic fallback rare".

   Notes:
   - This is both a performance fix and a design correction.
   - If `TransmissionReachability` exposes `producer_of(StateInterfaceId)`, that query
     can still be O(1), but the internal representation should not be organised around the idea
     that `TransmissionAnalysis::StateInterfaceId` is the planner's main identity model.

3. Add `reserve()` and pre-sizing systematically.
   Files:
   - [`src/joint_map/joint_map_blueprint.cpp`](../src/joint_map/joint_map_blueprint.cpp)
   - [`src/joint_map/materialize_joint_map.cpp`](../src/joint_map/materialize_joint_map.cpp)
   - [`src/joint_map/transmission_reachability.cpp`](../src/joint_map/transmission_reachability.cpp)
   - [`src/joint_map/default_joint_map_builder.cpp`](../src/joint_map/default_joint_map_builder.cpp)

   Changes:
   - Reserve `required`, `traced`, `ready`, `result`, `scratch_fills[k]` where possible.
   - Reserve `input_slot_of`, `scratch_slot_of`, `input_seeds`, `stages`, gather/scatter arrays.
   - Replace ad hoc uniqueness builds that use repeated `std::find` with membership structures or
     sort/unique, depending on the semantic requirement.

4. Remove avoidable vector shifting in topological sort.
   File:
   - [`src/joint_map/joint_map_blueprint.cpp`](../src/joint_map/joint_map_blueprint.cpp)

   Changes:
   - Replace `ready.erase(ready.begin())` with an index-based cursor over a sorted vector, or use a
     min-heap if the code becomes simpler.

### Acceptance criteria

- No behaviour change in existing tests.
- `perf report` no longer shows dense-id hash map internals among the top offenders for the full
  pipeline benchmark.
- Allocator share drops materially relative to the baseline in
  [`docs/performance-analysis.md`](./performance-analysis.md).

## Phase 2 — Correct The Internal Keying Model

### Objective

Stop using `StateInterfaceDefinition` as the default internal lookup key after the planner has
established its own canonical local ids.

### Work items

1. Introduce an explicit internal key model.
   Files:
   - [`include/arm_kinematics/joint_map/joint_map_blueprint.hpp`](../include/arm_kinematics/joint_map/joint_map_blueprint.hpp)
   - supporting implementation files

   Changes:
   - Define explicit local key spaces for the layers that need them.
   - Make those key spaces independent from whether a value happened to have a
     `TransmissionAnalysis::StateInterfaceId`.
   - Allow optional back-references to analysis ids where they exist.

   Candidate shape:
   - layer-local ids such as `ReachabilityStateId` and `BlueprintStateId`
   - a later shared planner-layer id only if reachability and blueprint actually converge on the
     same canonical space
   - owned tables:
     - `local_ref -> StateInterfaceDefinition`
     - optional `local_ref -> analysis-local state id`
     - optional `analysis-local state id -> local_ref`

   The exact type is flexible; the important part is that:

   - later stages stop hashing full definitions on every lookup
   - each layer owns its own identity model unless there is a deliberate shared planner-layer id
   - registered analysis ids become an optional acceleration path, not the semantic centre

2. Reduce `StateInterfaceDefinition` hashing in `materialize_joint_map()`.
   Files:
   - [`src/joint_map/materialize_joint_map.cpp`](../src/joint_map/materialize_joint_map.cpp)
   - related headers

   Changes:
   - Replace scratch-slot assignment keyed by `StateInterfaceDefinition` with slots keyed by
     planner-local canonical state references.
   - Replace `resolve_scratch(def)` lookups with direct indices where the planner already knows the
     answer.

3. Keep public correctness semantics unchanged while reconsidering the scope of analysis-local ids.
   Files:
   - [`include/arm_kinematics/utilities/interface_id.hpp`](../include/arm_kinematics/utilities/interface_id.hpp)
   - [`include/arm_kinematics/joint_map/state_interface_definition.hpp`](../include/arm_kinematics/joint_map/state_interface_definition.hpp)
   - [`include/arm_kinematics/joint_map/transmission_types.hpp`](../include/arm_kinematics/joint_map/transmission_types.hpp)
   - [`include/arm_kinematics/joint_map/transmission_analysis.hpp`](../include/arm_kinematics/joint_map/transmission_analysis.hpp)

   Changes:
   - No hash-only equality change.
   - If internal canonicalisation makes string equality mostly cold, leave the public types alone.
   - Evaluate moving `StateInterfaceId` out of the broad `arm_kinematics` namespace and into
     `TransmissionAnalysis` as an analysis-local type alias or nested type, to make its scope
     honest.
   - Avoid exposing an analysis-local registration detail as though it were the package's global
     identity model for state interfaces.

### Acceptance criteria

- `materialize_joint_map()` no longer performs repeated hash lookups on symbolic definitions in its
  inner build loops.
- `memcmp` and unordered-map lookup frames tied to `StateInterfaceDefinition` usage materially drop
  in the benchmark profile.

## Phase 3 — Correct The Planner/Materialiser API Boundary

### Objective

Make the blueprint carry the information the materialiser actually needs, using planner-local
canonical ids rather than symbolic definitions wherever possible.

### Why this is in scope

This is an API design correction, not a speculative redesign. The current boundary leaks work
across stages:

- the planner already knows dependency order
- the planner effectively decides scratch usage
- the materialiser recomputes scratch slot assignment and re-resolves source references

That is avoidable.

### Work items

1. Enrich `JointMapBlueprint` with compiled build-time metadata.
   Files:
   - [`include/arm_kinematics/joint_map/joint_map_blueprint.hpp`](../include/arm_kinematics/joint_map/joint_map_blueprint.hpp)
   - [`src/joint_map/joint_map_blueprint.cpp`](../src/joint_map/joint_map_blueprint.cpp)

   Changes:
   - Move scratch-slot planning into `plan_joint_map()`.
   - Store concrete gather/scatter-ready references in the blueprint rather than symbolic
     definitions wherever possible.

2. Simplify `materialize_joint_map()` into mostly object construction.
   Files:
   - [`src/joint_map/materialize_joint_map.cpp`](../src/joint_map/materialize_joint_map.cpp)

   Changes:
   - Remove the "discover scratch slots by walking the segments again" step.
   - Consume precomputed plan metadata directly.

3. Decide whether to keep one blueprint type or split it.
   Decision point:
   - Option A: keep `JointMapBlueprint` and make it more compiled.
   - Option B: keep a descriptive blueprint and add a second `CompiledJointMapBlueprint`.

   Recommendation:
   - Prefer Option B if preserving the existing conceptual meaning of "blueprint" keeps the API
     clearer.
   - Prefer Option A if there are no real users of the descriptive intermediate form and the extra
     type would only add ceremony.

   The important design rule is not the number of types. It is that the materialiser should not
   need to rediscover information that planning already established.

### Acceptance criteria

- Materialisation becomes a straightforward translation from precomputed build metadata into
  `AffineJointMap`, `TransmissionJointMap`, and `CompositeJointMapStage`.
- Scratch slot assignment no longer requires symbolic-key reconstruction.
- Code complexity in `materialize_joint_map.cpp` drops even as performance improves.

## Phase 4 — Clean Up Secondary Mistakes

### Objective

Fix smaller inefficiencies that will remain after the main representation and API corrections.

### Work items

1. Improve duplicate handling in `DefaultJointMapBuilder::build_expected()`.
   File:
   - [`src/joint_map/default_joint_map_builder.cpp`](../src/joint_map/default_joint_map_builder.cpp)

   Changes:
   - Replace repeated `std::find`-based deduplication of `unknown_joints` with a more direct
     structure.

2. Audit "first occurrence wins" code paths for repeated map probes.
   Files:
   - [`src/joint_map/materialize_joint_map.cpp`](../src/joint_map/materialize_joint_map.cpp)
   - [`src/joint_map/transmission_reachability.cpp`](../src/joint_map/transmission_reachability.cpp)

   Changes:
   - Where semantics are "record first occurrence", use a data structure that expresses that
     directly and avoids duplicate probing.

3. Reduce transient small-vector allocation where candidate counts are tiny.
   Files:
   - reachability internals

   Changes:
   - Consider a small-buffer container for candidate producer lists if profiling still shows
     allocation pressure after Phases 1-3.

### Acceptance criteria

- These changes should be justified by follow-up profiling, not habit.
- They should not complicate the code disproportionately.

## Current Status

### Completed so far

1. Analysis-local versus layer-local state ids were separated explicitly.
   Completed:
   - `TransmissionAnalysis::StateInterfaceId` is analysis-local only.
   - `TransmissionReachability::StateInterfaceId` is reachability-local.
   - `JointMapBlueprint` uses blueprint-local ids.

   Result:
   - the code no longer implies that one `StateInterfaceId` type is a package-wide semantic
     identity for state interfaces

2. Phase 1 dense-id fixes were completed in the obvious hotspots.
   Completed:
   - `plan_joint_map()` now uses vector-indexed `dependents`, `in_degree`, and `stage_index_of`
   - Kahn processing no longer uses `ready.erase(ready.begin())`
   - `TransmissionReachability` now stores `producer_assignment_` and pass-local candidate state in
     vector-indexed storage for registered interfaces

   Result:
   - these changes were correct and materially improved the original transmission-heavy benchmark
     cases before the later blueprint API experiments

3. `materialize_joint_map()` was corrected to stop re-hashing symbolic definitions in the hottest
   registered-interface paths.
   Completed:
   - registered scratch-slot assignment and gather/scatter now use dense indexed storage
   - symbolic `StateInterfaceDefinition` lookup is now limited more narrowly to bare-definition
     cases and boundary handling

4. `JointMapBlueprint` now owns an explicit blueprint-local canonical table.
   Completed:
   - blueprint-local ids are backed by a blueprint-owned `Order<>`
   - blueprint state records carry an optional back-reference to
     `TransmissionAnalysis::StateInterfaceId`

   Result:
   - the identity model is more honest
   - however, the current planner-side population strategy is too eager and has caused a measurable
     planning regression

5. `TransmissionAnalysis` now has analysis-local canonical interface-name ids.
   Completed:
   - added `InterfaceKindId`
   - added `Order<InterfaceId, InterfaceKindId>`
   - added canonical `(JointId, InterfaceKindId)` state keys
   - moved part of `TransmissionReachability`'s registered-member affine probing onto this
     canonical layer

   Result:
   - this establishes the right internal/public split for interface names
   - it does not, by itself, resolve the current planner regression

### New benchmark coverage

The benchmark suite now includes a pure direct-input rearrangement case:

- `InputReorder(N)` — no transmissions, no affine links, all inputs registered directly, outputs
  are the same interfaces in reverse order

This case is important because it isolates the workload that this remediation has actually been
optimising for: correcting planner/materialiser overhead in the absence of transmission semantics.

Current CPU means from that benchmark:

- `BM_Reachability_InputReorder/128`: about `4.36 us`
- `BM_PlanJointMap_InputReorder/128`: about `23.89 us`
- `BM_Materialize_InputReorder/128`: about `5.32 us`
- `BM_FullPipeline_InputReorder/128`: about `42.04 us`

Interpretation:

- `Reachability` is not the dominant cost for the direct-rearrangement case
- `Materialize` is not the dominant cost either
- `PlanJointMap` currently dominates the no-transmission workload

That makes the next optimisation target clear: the current planner-side blueprint canonicalisation
strategy is still doing too much work.

### Immediate next step

Keep the newer identity-model corrections, but reduce planner overhead by changing when
blueprint-local canonicalisation happens.

Specifically:

- do not eagerly canonicalise every transmission input/output through blueprint-local ids during
  `plan_joint_map()`
- keep blueprint-local canonical ids only where they actually remove repeated symbolic work at the
  planner/materialiser boundary
- keep analysis-local dense ids explicit where the planner already has them and where no symbolic
  recovery is required

The direct `InputReorder` benchmark should be treated as the primary acceptance check for that
next pass.

## Validation Strategy

### Benchmarks

Use the existing benchmark target from [`docs/performance-analysis.md`](./performance-analysis.md):

- `BM_PlanJointMap_*`
- `BM_Materialize_*`
- `BM_FullPipeline_*`
- `BM_*_InputReorder*` for the direct-input control case

Re-run after each phase rather than batching all changes together.

### Profiling checks

After each phase, confirm:

- total time change
- allocator share change
- whether hash-table internals remain visible in the top profile rows
- whether `memcmp` associated with symbolic identifier equality is still prominent

### Functional checks

Run the existing joint-map tests after each phase:

- [`test/joint_map/test_default_joint_map_builder.cpp`](../test/joint_map/test_default_joint_map_builder.cpp)
- [`test/joint_map/test_joint_map_blueprint.cpp`](../test/joint_map/test_joint_map_blueprint.cpp)
- [`test/joint_map/test_materialize_joint_map.cpp`](../test/joint_map/test_materialize_joint_map.cpp)
- [`test/joint_map/test_transmission_reachability.cpp`](../test/joint_map/test_transmission_reachability.cpp)
- [`test/joint_map/test_transmission_analysis.cpp`](../test/joint_map/test_transmission_analysis.cpp)
- [`test/joint_map/test_transmission_analysis_urdf_import.cpp`](../test/joint_map/test_transmission_analysis_urdf_import.cpp)

### Review questions after each phase

1. Did this remove work, or only move it?
2. Did this simplify the code's data model?
3. Did this make the API boundary more honest about what information is already known?
4. Is the measured gain large enough to justify the extra structure?

## Recommended Implementation Order

1. Phase 1 dense-id vectorisation and reserve work.
2. Phase 2 internal keying cleanup.
3. Phase 3 planner/materialiser API correction.
4. Phase 4 secondary cleanup only where profiling still justifies it.

This order matters. Phase 1 and Phase 2 will change what the real hot spots are. It would be a
mistake to redesign the whole builder API before seeing what remains after the obvious structural
mistakes are gone.

## Expected Outcome

If this plan succeeds:

- allocator overhead should stop dominating the builder profile
- dense-id build logic should largely stop paying hash-table costs
- symbolic definitions should become boundary types rather than hot-path lookup keys
- `materialize_joint_map()` should become simpler because the planner hands it better data

What this plan does **not** promise is that builder cost becomes irrelevant in all scenarios.
If the builder is still too expensive after these corrections, the next question should be whether
the system is rebuilding joint maps too often, not whether more micro-optimisation can rescue the
current lifecycle.
