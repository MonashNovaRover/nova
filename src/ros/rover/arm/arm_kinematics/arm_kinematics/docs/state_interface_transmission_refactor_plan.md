# Refactor: State-Interface-Based Transmission Model

## Context

The previous transmission model defined transmissions between **joints**, parameterized by `JointQuantity` (Position/Velocity) and `PropagationDirection` (Forward/Reverse). This model is too rigid:

- A single transmission cannot mix quantities (e.g., position-in / effort-out).
- `TransmissionModel` had to advertise capability via `(quantity, direction)` flags, complicating planning.
- Direction is fundamentally a property of which side is consumed vs. produced — encoding it as a runtime parameter duplicates information already implicit in the input/output sets.

The new model defines transmissions over **state interfaces** (`StateInterfaceDefinition = (JointId, InterfaceId)` where `InterfaceId` is e.g. `"position"`, `"velocity"`, `"effort"`). A transmission is a directed many-to-many mapping from a set of input state interfaces to a set of output state interfaces. Direction and quantity are emergent — not parameters.

This refactor is partially applied in two recent WIP commits:

- `52f96c3f` removed `JointQuantity` and `PropagationDirection` and migrated `TransmissionAnalysis` to `StateInterfaceId`. The codebase no longer compiles — many headers and impls still reference the removed types.
- `3a082eb7` introduced a `TransmissionSubgraph` skeleton intended as the build-time graph-analysis utility for joint map builders. Empty so far.

The goal of this plan: complete the migration cleanly, finalize `TransmissionSubgraph` as the central planning utility, and re-base joint map builders / runtime types / ros2_control wrappers on it.

---

## Core Model

### `StateInterfaceDefinition` (already correct — keep)
`(JointId, InterfaceId)`. Hashable, equatable. The canonical "node" of the transmission graph for non-affine transmissions.

### `TransmissionInstance` (in `TransmissionAnalysis`)
- `model_id`
- `input_ids: vector<StateInterfaceId>`
- `output_ids: vector<StateInterfaceId>`
- `name`

Direction is inherent: this instance reads `input_ids` and writes `output_ids`. A transmission that should support both directions is registered as **two** `TransmissionInstance`s pointing to the appropriate `TransmissionModel`(s).

### `AffineTransmission` — REVERT to joint-level
The current state-interface form is a step in the wrong direction. A mimic relationship is intrinsically a joint-to-joint invariant (one joint mirrors another), and applies uniformly across whatever interfaces (position, velocity, …) the planner happens to need — but the *projection rule* depends on the interface type.

```cpp
struct AffineTransmission {
  JointId source_joint_id;
  JointId target_joint_id;
  float multiplier;
  float offset;
};
```

`TransmissionSubgraph` is responsible for projecting these joint-level affine relationships onto the specific interface(s) required by a given build request, using the projection rules described next.

### `AffineProjectionRule` (new) and the projection registry on `TransmissionAnalysis`
Different interfaces project an affine joint-level relationship differently:
- **Position**: `q_target = m * q_source + o` — direct
- **Velocity**: `qdot_target = m * qdot_source` — offset drops under d/dt
- **Effort** (ideal energy-conserving coupling): `tau_source = m * tau_target` — magnitude scales by the reciprocal and the relationship runs target → source

A rule encodes how to derive the projected affine relationship for a given interface from the underlying joint-level `AffineTransmission`'s `(multiplier, offset)`:

```cpp
struct AffineProjectionRule {
  // Effective relationship:
  //   projected_target_iface = effective_m * projected_source_iface + effective_o
  // where:
  //   effective_m = (use_reciprocal_multiplier ? 1.0f / multiplier : multiplier) * multiplier_scale
  //   effective_o = offset * offset_scale
  // If reverse_direction is true, the source/target roles swap before applying
  // (i.e. projected_source_iface = AffineTransmission::target_joint_id's interface).
  float multiplier_scale = 1.0f;
  float offset_scale = 1.0f;
  bool use_reciprocal_multiplier = false;
  bool reverse_direction = false;
};
```

`TransmissionAnalysis` owns an `InterfaceId`-keyed map of these rules and **populates default entries on construction** for the common interface ids. Callers can override or add more via `set_affine_projection_rule(InterfaceId, AffineProjectionRule)`.

Built-in defaults:

| InterfaceId | `multiplier_scale` | `offset_scale` | `use_reciprocal_multiplier` | `reverse_direction` | Effective relation |
|---|---|---|---|---|---|
| `"position"` | 1.0 | 1.0 | false | false | `q_b = m·q_a + o` |
| `"velocity"` | 1.0 | 0.0 | false | false | `v_b = m·v_a` |
| `"effort"`   | 1.0 | 0.0 | true  | true  | `τ_a = m·τ_b` |
| `"acceleration"` | 1.0 | 0.0 | false | false | `a_b = m·a_a` |

An interface with no registered rule is treated as "affine does not propagate" — the subgraph will not use mimic relationships to satisfy it, and a missing input that could only be reached via affine projection through that interface will surface as missing.

### `TransmissionModel` (already correct — keep)
```cpp
virtual unique_ptr<const ComputeTransmission> build(
  span<const StateInterfaceId> input_ids,
  span<const StateInterfaceId> output_ids) const = 0;
```
No quantity, no direction. The model decides what to compute purely from which interfaces are listed in inputs vs outputs.

### `ComputeTransmission` (already correct — keep)
Unchanged: `compute(inputs, outputs, scratch)`.

### `TransmissionAnalysis` (cleanup)
- **Knows nothing about URDF, ros2_control, or mimic joints** — purely a typed graph data structure.
- Holds: joint order, state-interface order, models, transmissions (state-interface-edged), affine transmissions (joint-edged after the revert above), and the `InterfaceId → AffineProjectionRule` registry.
- API stays close to current shape; remove the `StateInterfaceId` overload of `add_affine_transmission` and replace with the joint-level form. Add `set_affine_projection_rule(InterfaceId, AffineProjectionRule)` and a corresponding query.

---

## TransmissionSubgraph (the central new utility)

A reusable, unopinionated, mutable data structure that captures the relevant subgraph of a `TransmissionAnalysis` for a given (inputs, outputs) request, supports incremental modification, and answers planning queries. Builders consume it; it does not embed any builder policy. Pass by const ref when used as input to other utilities.

**File:** `include/arm_kinematics/joint_map/transmission_subgraph.hpp` + `src/joint_map/transmission_subgraph.cpp`

### Internal state (proposed fields)
- `const TransmissionAnalysis & analysis_` (held by reference)
- Working set of **known** state interfaces (initially the requested inputs)
- Working set of **needed** state interfaces (initially the requested outputs)
- Per-interface **affine equivalence groups**: for each `InterfaceId` actually relevant to this subgraph, the set of joint equivalence classes closed under `analysis.affine_transmissions()` — materialized lazily as the planner walks
- Set of **selected** `TransmissionInstanceId`s with their consumed/produced state interfaces
- Set of **pruned** transmissions (e.g., transmissions whose only output is now already known)
- Optional: cached topological order of selected transmissions

### Public surface (proposed)
```cpp
class TransmissionSubgraph {
public:
  TransmissionSubgraph(
    const TransmissionAnalysis & analysis,
    span<const StateInterfaceId> initial_inputs,
    span<const StateInterfaceId> initial_outputs);

  // Mutation — incrementally extend the known/needed sets.
  // Any mutation invalidates all previously-returned spans from query methods.
  void add_input(StateInterfaceId);
  void add_output(StateInterfaceId);

  // Queries (analysis-only — no execution-ordering output).
  bool is_complete() const noexcept;                          // all needed outputs derivable
  span<const StateInterfaceId> missing_inputs() const noexcept;
  span<const StateInterfaceId> known_inputs() const noexcept;
  span<const StateInterfaceId> needed_outputs() const noexcept;
  span<const TransmissionInstanceId> selected_transmissions() const noexcept;

  bool produces(StateInterfaceId) const noexcept;
  // For each selected TransmissionInstance, what interfaces does the
  // subgraph consume from / produce into it (in this plan)?
  span<const StateInterfaceId> consumed_by(TransmissionInstanceId) const noexcept;
  span<const StateInterfaceId> produced_by(TransmissionInstanceId) const noexcept;
  // Affine projection participants for a given interface — the joint
  // equivalence classes closed under affine_transmissions, projected via
  // the AffineProjectionRule registered for this interface.
  span<const JointId> affine_class(InterfaceId, JointId representative) const noexcept;
  // (more queries as needed by builders / tests)
};
```

The subgraph deliberately does **not** expose an "ordered execution stages" method. Builders are responsible for emitting their own staging by walking the subgraph through these queries — the subgraph is a pure analysis utility, not a planning pipeline.

### Invariants
- Never holds dangling references — all `JointId`/`StateInterfaceId`/`TransmissionInstanceId` values exist in `analysis_`.
- Selected transmissions form a DAG (no cycles).
- Each currently-needed state interface has at most one chosen producer (when a second producer is found, the subgraph either picks deterministically or marks the request ambiguous — surfaced via a query).
- `known_inputs ∩ missing_inputs == ∅`.
- `add_input` of an interface that was missing moves it to known and prunes its previous producer if and only if the producer is no longer needed by any other path.

### Algorithm sketch
1. Initialize known = initial_inputs, needed = initial_outputs.
2. For each needed interface not yet known: walk backward through `analysis_.transmissions()` to find candidates whose `output_ids` intersect needed.
3. For each candidate transmission, push its `input_ids` into needed (unless already known).
4. Within a single `InterfaceId`, materialize the affine equivalence group projected from `analysis_.affine_transmissions()`: a missing interface that lives in the same group as a known interface is satisfiable via an affine stage.
5. Repeat until needed is empty (complete) or no progress is possible (missing_inputs is non-empty).
6. `add_input` triggers a forward pass: a newly-known interface may unlock transmissions whose all-but-this-input were already known.

### Why not a snapshot/freeze API
The user's intent is that `TransmissionSubgraph` is a building block — a strongly-invariant utility that anything in the construction pipeline can hold and query, not a pipeline stage with its own lifecycle. Pass it by `const &` to anything that needs read-only views.

---

## JointMapBuilder

Stays a virtual interface. Strategy for missing inputs lives entirely in subclasses (open/closed). FK plugins are free to provide their own builders.

**File:** `include/arm_kinematics/joint_map/joint_map_builder.hpp`

### Canonical signature
```cpp
class JointMapBuilder {
public:
  virtual ~JointMapBuilder() = default;
  [[nodiscard]] virtual tl::expected<JointMap, JointMapBuildError> build_expected(
    span<const StateInterfaceId> inputs,
    span<const StateInterfaceId> outputs) const = 0;
};
```

`StateInterfaceId` is the canonical boundary type, not `StateInterfaceDefinition`. Callers resolve names against the analysis up front. A `NamedStateInterfaceDefinition` convenience overload can be added later but is **out of scope** for this refactor.

### `JointMapBuildError`
```cpp
struct JointMapBuildError {
  enum class Kind { NoPlan, Ambiguous, Invalid, MissingInputs };
  Kind kind = Kind::NoPlan;
  std::string message;
  std::vector<StateInterfaceId> missing_inputs;        // populated when kind == MissingInputs
  std::vector<MissingInputResolution> resolutions;     // optional rich-error hints; may be empty
};

// Reusable, self-contained helper — not a member of subgraph or builder.
// Lives somewhere like include/arm_kinematics/joint_map/missing_input_resolution.hpp.
struct MissingInputResolution {
  StateInterfaceId missing;
  std::vector<std::vector<StateInterfaceId>> alternative_input_sets;
};
[[nodiscard]] std::vector<MissingInputResolution> compute_missing_input_resolutions(
  const TransmissionAnalysis & analysis,
  span<const StateInterfaceId> missing_inputs);
```

### `DefaultJointMapBuilder` (and `TransmissionAnalysisJointMapBuilder`)
- Construct a `TransmissionSubgraph` from the request.
- If `!subgraph.is_complete()`: return failure with `kind = MissingInputs`, the missing interfaces, and resolution hints from `compute_missing_input_resolutions(analysis, subgraph.missing_inputs())`.
- Otherwise: walk the subgraph (using its query API) and emit a `JointMap` directly. The builder constructs the appropriate concrete runtime type:
  - Pure-affine plan (no transmissions selected) → `AffineJointMap`.
  - Single transmission, no affine stages → a transmission-backed joint map directly.
  - Mixed → `CompositeJointMap` over affine + transmission segments.
  - The builder is free to extract sub-helpers as needed; this is normal code, not a single inline function.

`DefaultJointMapBuilder` and `TransmissionAnalysisJointMapBuilder` likely collapse into a single class once the only flow goes through `TransmissionSubgraph`. Decide during step 5 of the execution order; if both survive, the distinction must be documented.

Error messages should report exactly what is missing, and what would need to be supplied to resolve the issue (if multiple possible resolutions exist, this should be clearly communicated in error messaging).

In a correctly-configured robot, missing inputs only happen when the user has forgotten to expose required state interfaces in their ros2_control setup — it is a configuration error, not a recoverable runtime case. The builder fails loudly so the user can see exactly what they need to fix. A future addition will be controller-side helpers that automatically derive the required state interface set from a `JointMap`'s declared inputs and request them through the controller's interface configuration, so users never have to manually figure out the requirements by reading errors and editing config.

### Custom builders (future)
A specialized FK plugin builder can wrap the default and either:
- Supply default values for interfaces in `subgraph.missing_inputs()` (perhaps via a small "default value source" interface) before extracting `ordered_stages`, or
- ~~Re-export the missing list to upstream ros2_control controller code so the controller can request the additional state interfaces.~~ //< not possible, as required interfaces need to be already defined.

This plan does **not** implement these strategies — it just leaves the seam clean.

---

## Plan / Runtime Types

The legacy `make_*_plan_expected` helper functions and the intermediate `JointMapPlan`/`AffinePlan`/`TransmissionPlan` plan structs are scaffolding from the previous (joint+quantity+direction) design. They are **deleted entirely**. The builder constructs concrete `JointMap` runtime types directly from the subgraph queries — extracting helpers as needed. Don't impose "everything inline in one function" — use judgment.

**File:** `include/arm_kinematics/joint_map/transmission_plan.hpp` + cpp — **DELETE**.

**Files:** `include/arm_kinematics/joint_map/transmission_joint_map.hpp` + cpp
- Remove the `compile_transmission_plan_expected` API entirely.
- Replace with a constructor (or factory) on `TransmissionJointMap` that takes its inputs directly: the resolved sequence of `(TransmissionInstance, consumed_state_interface_ids, produced_state_interface_ids)` triples. The builder produces these by walking the subgraph.

**Files:** `include/arm_kinematics/joint_map/composite_joint_map.hpp` + cpp
- Remove the `compile_joint_map_plan_expected` API entirely.
- `CompositeJointMap` keeps its existing run-time shape (a sequence of segment joint maps over output index ranges), but is now constructed directly from the segments the builder emits — no intermediate plan struct.

**Files:** `include/arm_kinematics/joint_map/affine_joint_map.hpp` + cpp
- No structural change. Verify it still compiles after the joint-level `AffineTransmission` revert. The builder constructs `AffineJointMap` directly from sources/multipliers/offsets it computes from the subgraph.

---

## ros2_control wrapper

**File:** `src/joint_map/transmission_analysis_import.cpp` (+ corresponding headers)

Critical principle (from the user): `TransmissionAnalysis` must never know about ros2_control. The wrappers go through the **same** `TransmissionModel` / `ComputeTransmission` mechanisms as any other transmission. No special path.

### `Ros2ControlPluginTransmissionModel`
- Implements `TransmissionModel`. Owns the loaded `transmission_interface::Transmission` plugin instance.
- Loaded **once at import time**: the URDF importer instantiates the plugin so it can probe which (direction × quantity) combinations the plugin actually supports, and registers `TransmissionInstance` entries accordingly. The plugin instance stays loaded for the lifetime of the model — it is the model.
- `build(input_ids, output_ids)` creates a `Ros2ControlPluginTransmissionCompute` that holds preallocated handle storage and references the model's plugin instance. The model decides what plugin call to dispatch to from the input/output state interface ids alone — no quantity / direction fields.
- Drop all internal `JointQuantity` / `PropagationDirection` fields.

### `Ros2ControlPluginTransmissionCompute`
- Owns its preallocated handle storage; references the plugin instance owned by the model.
- `compute(inputs, outputs, scratch)`: copy inputs into handles, call the plugin, copy outputs out. Allocation-free.

### URDF importer (lives outside `TransmissionAnalysis`)
- Parses URDF and ros2_control transmission XML.
- For each ros2_control transmission, instantiates the plugin via the loader, probes its supported combinations, and registers one `TransmissionInstance` per supported (direction × quantity) combination — each pointing at the same `Ros2ControlPluginTransmissionModel` (one model per ros2_control transmission, shared across the up-to-4 instances).
- Mimic joints become `TransmissionAnalysis::add_affine_transmission(JointId, JointId, multiplier, offset)` calls. The importer also registers `AffineProjectionRule` entries for the interface ids it expects affine propagation to apply to (typically `"position"`, `"velocity"`, possibly `"effort"`). The importer is the only place that knows mimic joints exist.

---

## Files to modify / delete

**Keep / verified correct:**
- `include/arm_kinematics/joint_map/state_interface_definition.hpp`
- `include/arm_kinematics/joint_map/transmission_types.hpp` (already minimal)
- `include/arm_kinematics/joint_map/transmission_model.hpp`
- `include/arm_kinematics/joint_map/compute_transmission.hpp`
- `include/arm_kinematics/joint_map/affine_joint_map.hpp/.cpp` (verify compiles)
- `include/arm_kinematics/utilities/order.hpp` (recently extended; correct)

**Delete:**
- `include/arm_kinematics/joint_map/transmission_plan.hpp` and `src/joint_map/transmission_plan.cpp` — entirely.

**Modify substantially:**
- `include/arm_kinematics/joint_map/transmission_analysis.hpp/.cpp` — revert `AffineTransmission` to `JointId`-edged; add `AffineProjectionRule` storage and accessors
- `include/arm_kinematics/joint_map/transmission_subgraph.hpp/.cpp` — flesh out fields, invariants, queries, algorithm
- `include/arm_kinematics/joint_map/transmission_joint_map.hpp/.cpp` — remove `compile_transmission_plan_expected`; replace with direct constructor / factory the builder calls
- `include/arm_kinematics/joint_map/composite_joint_map.hpp/.cpp` — remove `compile_joint_map_plan_expected`; constructor builds segments directly
- `include/arm_kinematics/joint_map/joint_map_builder.hpp` — finalize canonical signature (StateInterfaceId-based); define `JointMapBuildError`
- `include/arm_kinematics/joint_map/default_joint_map_builder.hpp/.cpp` — use `TransmissionSubgraph`; fail-loudly-with-rich-error strategy
- `include/arm_kinematics/joint_map/transmission_analysis_joint_map_builder.hpp/.cpp` — same; consider collapsing into `DefaultJointMapBuilder`
- `src/joint_map/transmission_analysis_import.cpp` — drop `JointQuantity`/`PropagationDirection` internals; restructure `Ros2ControlPluginTransmission*` to load plugin once at import and dispatch from state interface ids; route mimic joints to joint-level `add_affine_transmission`; register affine projection rules

**New:**
- `include/arm_kinematics/joint_map/missing_input_resolution.hpp` + cpp — reusable, self-contained `compute_missing_input_resolutions(analysis, missing_inputs)` helper used by builders to populate `JointMapBuildError::resolutions`.

**Tests:**
- Split `test/forward/eigen/test_eigen_fk_mapper.cpp` into focused files (e.g. `test_eigen_fk_reorder.cpp`, `test_eigen_fk_mimic.cpp`, `test_eigen_fk_transmission.cpp`). Update all sites to the new API; remove all `JointQuantity` / `PropagationDirection` references.
- New `test/joint_map/test_transmission_subgraph.cpp` (no FK dependency) — unit tests for the subgraph against synthetic `TransmissionAnalysis` instances.
- New `test/joint_map/test_missing_input_resolution.cpp` — unit tests for the resolution helper.

---

## Suggested execution order

1. **TransmissionAnalysis cleanup.** Revert `AffineTransmission` to `JointId` form. Add `AffineProjectionRule` struct + the `InterfaceId → AffineProjectionRule` registry on `TransmissionAnalysis` (with set/query API). Confirm `transmission_analysis.hpp` knows nothing about URDF / mimic / ros2_control.
2. **Delete legacy plan scaffolding.** Delete `transmission_plan.hpp/.cpp`. Delete the `compile_*_plan_expected` APIs from `transmission_joint_map` and `composite_joint_map`. Strip every remaining reference to `JointQuantity` / `PropagationDirection` from headers and impls. The codebase is intentionally non-compiling between this step and step 5; that's fine.
3. **Define `JointMapBuilder` shape.** Finalize the canonical `build_expected(span<const StateInterfaceId>, span<const StateInterfaceId>)` signature. Define `JointMapBuildError` and the `MissingInputResolution` helper header (impl can stub for now).
4. **Build out `TransmissionSubgraph`.** Fields, invariants, algorithm, queries. The subgraph reads `AffineProjectionRule`s from analysis when projecting affine relationships per interface. Add focused unit tests in `test/joint_map/test_transmission_subgraph.cpp` against synthetic `TransmissionAnalysis` instances — no FK dependency.
5. **Reconnect runtime joint maps.** Give `TransmissionJointMap` and `CompositeJointMap` direct constructors / factories the builder can call without any plan struct intermediary.
6. **Rewire `DefaultJointMapBuilder`** (and decide whether `TransmissionAnalysisJointMapBuilder` collapses into it) to construct a `TransmissionSubgraph`, fail loudly with rich error on incomplete plans (using `compute_missing_input_resolutions`), and otherwise emit a `JointMap` directly by walking the subgraph. Codebase compiles again from this step on.
7. **Restructure ros2_control wrappers.** `Ros2ControlPluginTransmissionModel` loads its plugin once at import; `Compute` holds preallocated handles. Move mimic import to use joint-level `add_affine_transmission` and register the `AffineProjectionRule`s for `"position"` / `"velocity"` / etc.
8. **Test split + updates.** Split `test_eigen_fk_mapper.cpp` into focused files. Update all call sites to the new API. Add `test_missing_input_resolution.cpp`.
9. **Build, run all tests.**

---

## Verification

- build succeeds.
- tests succeed — all existing tests pass after migration:
  - Reorder tests
  - Mimic chain tests (now using joint-level affine + per-interface materialization)
  - Forward and reverse single-transmission tests (now expressed as two `TransmissionInstance`s)
  - Multi-input/multi-output grouped transmission
- New tests:
  - `TransmissionSubgraph` unit tests against a synthetic `TransmissionAnalysis` (no FK):
    - completable plan with pure transmission graph
    - completable plan that requires affine projection through mimic joints (position rule)
    - completable plan that requires affine projection with the offset dropped (velocity rule)
    - completable plan that requires affine projection with reversed direction (effort rule)
    - missing-inputs reporting when outputs are unreachable
    - `add_input` unblocks a previously-incomplete plan
    - mixed-quantity transmission (position-in / effort-out)
    - ambiguity reporting when multiple producers exist for one needed interface
  - `compute_missing_input_resolutions` unit tests:
    - empty missing list → empty resolutions
    - single missing interface with one producing transmission → one resolution with that transmission's input set
    - missing interface with multiple producing transmissions → one resolution with multiple alternative input sets
  - `DefaultJointMapBuilder` returns rich `JointMapBuildError` when inputs are missing, including resolution hints.

Existing tests are split out of `test_eigen_fk_mapper.cpp` into focused files as part of this work.

---

## Open items / out of scope

- `NamedStateInterfaceDefinition` convenience overloads on `JointMapBuilder` (deferred — canonical `StateInterfaceId` API only for now).
- Default-value-supplying joint map builder strategies (the seam exists; no concrete implementation).
- ros2_control controller helpers for sourcing missing state interfaces (acknowledged use case; future work).
- Ambiguity resolution policies in `TransmissionSubgraph` beyond "first wins" / "report ambiguous" (refine with concrete tests as they appear. Do try to report ambiguity).

