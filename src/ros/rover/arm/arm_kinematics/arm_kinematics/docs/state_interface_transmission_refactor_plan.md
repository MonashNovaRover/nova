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
  float multiplier;   // must be non-zero — see precondition below
  float offset;
};
```

**Precondition: `multiplier != 0`**, validated on `add_affine_transmission`. Allowing `multiplier == 0` would model `target = offset` (a constant value with no actual link to `source`) — that isn't really a mimic relationship and doesn't belong in `AffineTransmission`. Joints that are always at a constant value are handled by a different mechanism (the future default-value-source seam — a JointMapBuilder strategy listed in "Open items" that supplies fallback values for missing inputs, or the caller supplies the value as an input directly). With `multiplier != 0`, every affine relationship is guaranteed bidirectional, which keeps the affine-group semantics clean (see next).

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
- Holds: joint order, state-interface order, models, transmissions (state-interface-edged), affine transmissions (joint-edged after the revert above), the `InterfaceId → AffineProjectionRule` registry as `std::unordered_map<InterfaceId, AffineProjectionRule>` (using `InterfaceId`'s existing FNV1a hash; `Order<>` is the wrong tool here — no stable internal IDs are needed), and the **affine group** index (see below).
- Maintains an **inverse index** from `StateInterfaceId → vector<TransmissionInstanceId>` of transmissions whose `output_ids` contain that interface. Built incrementally as `add_transmission` is called. Lets the subgraph algorithm answer "who produces X?" in expected O(1) instead of O(num_transmissions × num_outputs).
- API stays close to current shape; remove the `StateInterfaceId` overload of `add_affine_transmission` and replace with the joint-level form. Add `set_affine_projection_rule(InterfaceId, AffineProjectionRule)` and a corresponding query.
- **Duplicate output state interfaces are allowed** in `TransmissionInstance::output_ids` (and in builder requests). Asking for the same value in multiple places is a legitimate use case and is not an error.

### Affine group queries (root-joint identification)
Connected components of the joint graph induced by `analysis.affine_transmissions()` are first-class. Because `multiplier != 0` is enforced, every edge is bidirectional and each component has the property that **defining any single joint in the group defines all of them** (with composed `(m, o)` derived by walking edges).

Groups are identified by their **root joint** — the union-find representative — which is itself a valid `JointId` and can be named in error messages. Every joint is in a group (possibly a trivial group of one, where the root is the joint itself), so no sentinel id is needed.

```cpp
// On TransmissionAnalysis:
// Returns the root joint of the affine group containing j. If j has no affine
// relationships, returns j itself.
[[nodiscard]] JointId affine_root_of(JointId j) const noexcept;
// All members of the group whose root is `root`. Always contains at least `root`.
[[nodiscard]] span<const JointId> affine_group_members(JointId root) const noexcept;
```

The index is maintained incrementally by union-find as `add_affine_transmission` is called — each new edge merges two groups. This makes both the subgraph algorithm and the missing-input resolution helper much simpler and more efficient than walking edges on demand.

---

## TransmissionSubgraph (the central new utility)

A reusable, unopinionated, mutable data structure that captures the relevant subgraph of a `TransmissionAnalysis` for a given (inputs, outputs) request, supports incremental modification, and answers planning queries. Builders consume it; it does not embed any builder policy. Pass by const ref when used as input to other utilities.

**File:** `include/arm_kinematics/joint_map/transmission_subgraph.hpp` + `src/joint_map/transmission_subgraph.cpp`

### Internal state (proposed fields)
- `const TransmissionAnalysis & analysis_` (held by reference)
- Working set of **known** state interfaces (initially the requested inputs)
- Working set of **needed** state interfaces (initially the requested outputs)
- Per-output **producer assignment**: `unordered_map<StateInterfaceId, StateInterfaceProducer>` populated as the algorithm runs; backs `producer_of()` in O(1)
- Topologically-ordered list of **selected** `TransmissionInstanceId`s
- Set of **ambiguous** state interfaces (each with its competing candidates)
- Cached list of **missing inputs** — needed interfaces with no viable producer

(Affine groups themselves live on `TransmissionAnalysis`; the subgraph just looks them up by `JointId`. Per-interface "affine equivalence groups" are not stored separately on the subgraph — the joint-level groups plus the per-interface projection rules cover the same information without duplication.)

### Public surface (proposed)
```cpp
// What produces a given StateInterfaceId in the current plan?
// Each alternative carries only the data relevant to its case.
namespace producers {
  struct Input {
    size_t input_index;        // position in the request's inputs span
  };
  struct AffineProjection {
    StateInterfaceId source;   // the known interface providing the value
    float multiplier;          // composed across the chain, with the projection rule applied
    float offset;
  };
  struct Transmission {
    TransmissionInstanceId transmission;
  };
}
// std::monostate represents "not produced" (interface is missing or unrequested).
using StateInterfaceProducer = std::variant<
  std::monostate,
  producers::Input,
  producers::AffineProjection,
  producers::Transmission
>;

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
  bool is_complete() const noexcept;                          // all needed outputs derivable AND not ambiguous
  bool is_ambiguous() const noexcept;                         // any needed interface has multiple viable producers
  span<const StateInterfaceId> missing_inputs() const noexcept;
  span<const StateInterfaceId> known_inputs() const noexcept;
  span<const StateInterfaceId> needed_outputs() const noexcept;
  // All ambiguous interfaces with their competing candidates, accumulated across the
  // full algorithm pass — not just the first one encountered.
  struct AmbiguousInterface {
    StateInterfaceId interface;
    std::vector<StateInterfaceProducer> candidates;
  };
  span<const AmbiguousInterface> ambiguous_interfaces() const noexcept;

  // Returned in topological order: if transmission B consumes any output of
  // transmission A, A appears before B. Builders rely on this for emission.
  span<const TransmissionInstanceId> selected_transmissions() const noexcept;

  bool produces(StateInterfaceId) const noexcept;
  // The principal builder query: how is this state interface produced in the plan?
  // O(1) lookup. Returns std::monostate if the interface is missing, ambiguous,
  // or unrequested. Builders should check is_ambiguous() before walking outputs.
  StateInterfaceProducer producer_of(StateInterfaceId) const noexcept;
};
```

**Builder note on chained producers.** A `producers::AffineProjection` carries a `source` `StateInterfaceId` that is itself produced by *something* — usually a requested input, but possibly the output of a transmission earlier in the topological order. Builders consuming `producer_of()` results must `producer_of(source)` to find the actual data location. The recursion always terminates at `producers::Input` or `producers::Transmission`.

The subgraph deliberately does **not** expose an "ordered execution stages" method. Builders are responsible for emitting their own staging by walking the subgraph through these queries — the subgraph is a pure analysis utility, not a planning pipeline.

Algorithm execution timing (eager-in-constructor vs lazy-on-first-query) is an **implementation detail**. The plan does not prescribe one — pick whichever is most efficient at implementation time. The strong invariants below must hold immediately after any mutation regardless.

### Invariants
- Never holds dangling references — all `JointId`/`StateInterfaceId`/`TransmissionInstanceId` values exist in `analysis_`.
- Selected transmissions form a DAG (no cycles).
- `selected_transmissions()` is returned in topological order — dependencies before dependents.
- **Ambiguity is accumulated and surfaced via a query, never silently resolved.** The algorithm continues past ambiguous interfaces, marking each one with its competing candidates, and exposes the full set via `ambiguous_interfaces()`. `is_complete()` requires both `!is_ambiguous()` and an empty `missing_inputs()`.
- For ambiguous interfaces, `producer_of()` returns `std::monostate`. Non-ambiguous interfaces in the same plan still return their unique producer.
- `known_inputs ∩ missing_inputs == ∅`. If an interface appears in both `initial_inputs` and `initial_outputs`, it is treated as known (produced by `producers::Input`) and removed from needed.
- `add_input` of an interface that was missing moves it to known, recomputes affected reachability, and may resolve a previous ambiguity (by making one competing path no longer needed) or unblock previously-missing interfaces.

### Algorithm sketch
The algorithm is a **forward fixed-point** over viable producers, not a naive backward walk. A transmission is only a *candidate* producer for one of its outputs once **all of its inputs are themselves derivable** — otherwise it cannot run, so it cannot disambiguate or block other paths.

1. **Initialize.** `known = initial_inputs ∪ (initial_outputs ∩ initial_inputs)`. `needed = initial_outputs \ known`. For each interface that started in both, record `producers::Input{input_index}`.
2. **Compute viable transmissions and affine reach.** Iterate to a fixed point:
   - For every `TransmissionInstance T` in `analysis_.transmissions()`: if every interface in `T.input_ids` is in known, then every interface in `T.output_ids` becomes derivable from `T`.
   - For every joint `J` whose `(J, I)` is in known and whose `I` has a registered `AffineProjectionRule`: every other joint `J'` in `analysis.affine_group_members(analysis.affine_root_of(J))` has `(J', I)` derivable via affine projection. The composed `(multiplier, offset)` is built by walking the affine edges from `J` to `J'` and applying the projection rule (`use_reciprocal_multiplier`, `multiplier_scale`, `offset_scale`, and the `reverse_direction` source/target swap).
   - Each newly-derivable interface is added to known. Continue until no new interfaces are added.
   - **For each newly-derivable interface, count its viable producers.** A "viable producer" is a viable transmission or an affine projection whose source is known. If the count is 1, record that single producer in the producer-assignment map. If the count is > 1, record the interface as ambiguous (with the full candidate list) — but the algorithm does not stop. The interface is still considered known for the purpose of unblocking downstream transmissions, so the full graph reachability is still computed.
3. **Classify needed outputs.**
   - In known and not ambiguous → `producer_of()` returns the recorded producer; the output is satisfied.
   - In known and ambiguous → contributes to `ambiguous_interfaces()`; `producer_of()` returns `monostate`.
   - Not in known → contributes to `missing_inputs()`.
4. **Topological emission.** `selected_transmissions()` collects every transmission that produces at least one (non-ambiguous) needed output, transitively. Sort topologically: if `T2`'s inputs depend on any of `T1`'s outputs, `T1` precedes `T2`. (The dependency graph is a DAG by construction — every transmission's inputs are derivable from a strict subset of `known` at the time it became viable.)
5. **Mutation.** `add_input(I)` adds `I` to known, then re-runs the fixed point from step 2. Newly-viable transmissions and previously-ambiguous interfaces may resolve. `add_output(I)` adds `I` to needed and re-classifies.

This formulation has the nice property that "viability" is a fixed point — we never have to undo a candidate decision because of input unreachability. Ambiguity is detected accurately (only between *actually viable* paths) and is accumulated across the entire pass.

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
  enum class Kind {
    MissingInputs,  // needed outputs are not derivable from the given inputs
    Ambiguous,      // one or more needed interfaces have multiple viable producers
    Invalid,        // request is malformed (unknown interface ids, etc.)
  };
  Kind kind = Kind::MissingInputs;
  std::string message;

  // Populated when kind == MissingInputs.
  std::vector<StateInterfaceId> missing_inputs;
  std::vector<MissingInputResolution> resolutions;     // rich hints; may be empty

  // Populated when kind == Ambiguous (mirrors TransmissionSubgraph::AmbiguousInterface).
  std::vector<TransmissionSubgraph::AmbiguousInterface> ambiguous_interfaces;
};

// Reusable, self-contained helper — not a member of subgraph or builder.
// Lives somewhere like include/arm_kinematics/joint_map/missing_input_resolution.hpp.
struct MissingInputResolution {
  StateInterfaceId missing;

  // Each entry is a set of state interfaces that — if all supplied — would unblock
  // the missing interface via one specific transmission. Empty if no transmission
  // path exists.
  std::vector<std::vector<StateInterfaceId>> transmission_alternatives;

  // If the missing interface lives in a non-trivial affine group AND has a registered
  // projection rule, this is the root joint of that group. The user resolves the
  // missing interface by supplying any single (joint_in_group, missing.interface_id)
  // where joint_in_group is any joint reported by analysis.affine_group_members(affine_root).
  // If no affine resolution exists, affine_root is std::nullopt.
  std::optional<JointId> affine_root;
};

[[nodiscard]] std::vector<MissingInputResolution> compute_missing_input_resolutions(
  const TransmissionSubgraph & subgraph);
```

The helper takes the subgraph (not just the analysis) so it can:
- Use `subgraph.known_inputs()` to filter out trivially-already-supplied alternatives.
- Surface only the alternatives that would actually unblock progress in the current request context.
- Reach into both the transmission inverse index and the affine group index on the underlying analysis.

Note: **duplicate output state interfaces are allowed in builder requests** (asking for the same value in two output positions is legitimate) and never raise `Invalid`.

### `DefaultJointMapBuilder`
This is the **only** concrete builder in this refactor. The legacy `TransmissionAnalysisJointMapBuilder` is deleted — the previous distinction was an artifact of preserving the old implementation alongside a new one, and no longer applies once everything flows through `TransmissionSubgraph`.

```cpp
class DefaultJointMapBuilder : public JointMapBuilder {
public:
  explicit DefaultJointMapBuilder(const TransmissionAnalysis & analysis);

  [[nodiscard]] tl::expected<JointMap, JointMapBuildError> build_expected(
    span<const StateInterfaceId> inputs,
    span<const StateInterfaceId> outputs) const override;

private:
  const TransmissionAnalysis & analysis_;
};
```

Behavior of `build_expected`:
- Construct a `TransmissionSubgraph` from `(analysis_, inputs, outputs)`.
- If `subgraph.is_ambiguous()`: return failure with `kind = Ambiguous` and `ambiguous_interfaces` populated from `subgraph.ambiguous_interfaces()`. The message names each ambiguous interface and its candidate producers.
- If `!subgraph.is_complete()` (and not ambiguous): return failure with `kind = MissingInputs`, the missing interfaces, and resolution hints from `compute_missing_input_resolutions(subgraph)`.
- Otherwise: walk the subgraph (using `producer_of()` for each requested output, plus `selected_transmissions()` in topological order for staging) and emit a `JointMap` directly. For `producers::AffineProjection`, recursively call `producer_of(source)` to find the underlying data location (an `Input` or a `Transmission` output). The builder constructs the appropriate concrete runtime type:
  - Pure-affine plan (no transmissions selected) → `AffineJointMap`.
  - Single transmission, no affine stages → a transmission-backed joint map directly.
  - Mixed → `CompositeJointMap` over affine + transmission segments.
  - The builder is free to extract sub-helpers as needed; this is normal code, not a single inline function.

Error messages should report exactly what is missing, and what would need to be supplied to resolve the issue (if multiple possible resolutions exist, this should be clearly communicated in error messaging).

In a correctly-configured robot, missing inputs only happen when the user has forgotten to expose required state interfaces in their ros2_control setup — it is a configuration error, not a recoverable runtime case. The builder fails loudly so the user can see exactly what they need to fix. A future addition will be controller-side helpers that automatically derive the required state interface set from a `JointMap`'s declared inputs and request them through the controller's interface configuration, so users never have to manually figure out the requirements by reading errors and editing config.

### Custom builders (future)
FK plugins can subclass `JointMapBuilder` to provide alternative behaviors. The two anticipated extensions, neither of which is in scope for this refactor:
- A builder that supplies **default values** for interfaces in `subgraph.missing_inputs()` via a small "default value source" interface, then constructs the `JointMap` as if those values had been provided as inputs.
- A builder that emits a richer error report tailored to a specific FK plugin's domain.

The seam is clean: subclass `JointMapBuilder`, hold a reference to the analysis, build a `TransmissionSubgraph`, and react to its queries however the strategy requires.

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
- Mimic joints become `TransmissionAnalysis::add_affine_transmission(JointId, JointId, multiplier, offset)` calls. The importer is the only place that knows mimic joints exist. It does **not** need to register `AffineProjectionRule` entries for `"position"`/`"velocity"`/`"effort"`/`"acceleration"` — `TransmissionAnalysis` populates those defaults on construction. The importer only registers projection rules when overriding a default or adding a custom interface id.

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
- `include/arm_kinematics/joint_map/transmission_analysis_joint_map_builder.hpp` and `src/joint_map/transmission_analysis_joint_map_builder.cpp` — collapsed into `DefaultJointMapBuilder`.

**Modify substantially:**
- `include/arm_kinematics/joint_map/transmission_analysis.hpp/.cpp` — revert `AffineTransmission` to `JointId`-edged; add `AffineProjectionRule` storage and accessors; populate defaults in constructor
- `include/arm_kinematics/joint_map/transmission_subgraph.hpp/.cpp` — flesh out fields, invariants, queries, algorithm
- `include/arm_kinematics/joint_map/transmission_joint_map.hpp/.cpp` — remove `compile_transmission_plan_expected`; replace with direct constructor / factory the builder calls
- `include/arm_kinematics/joint_map/composite_joint_map.hpp/.cpp` — remove `compile_joint_map_plan_expected`; constructor builds segments directly
- `include/arm_kinematics/joint_map/joint_map_builder.hpp` — finalize canonical signature (StateInterfaceId-based); define `JointMapBuildError`
- `include/arm_kinematics/joint_map/default_joint_map_builder.hpp/.cpp` — use `TransmissionSubgraph`; fail-loudly-with-rich-error strategy; absorb any unique behavior previously in `TransmissionAnalysisJointMapBuilder`
- `src/joint_map/transmission_analysis_import.cpp` — drop `JointQuantity`/`PropagationDirection` internals; restructure `Ros2ControlPluginTransmission*` to load plugin once at import and dispatch from state interface ids; route mimic joints to joint-level `add_affine_transmission`
- `CMakeLists.txt` — remove deleted source files; add new `transmission_subgraph.cpp` and `missing_input_resolution.cpp`; register the new `test/joint_map/` test directory and the split eigen FK test files

**New:**
- `include/arm_kinematics/joint_map/missing_input_resolution.hpp` + cpp — reusable, self-contained `compute_missing_input_resolutions(analysis, missing_inputs)` helper used by builders to populate `JointMapBuildError::resolutions`.

**Tests:**
- Split `test/forward/eigen/test_eigen_fk_mapper.cpp` into focused files (e.g. `test_eigen_fk_reorder.cpp`, `test_eigen_fk_mimic.cpp`, `test_eigen_fk_transmission.cpp`). Update all sites to the new API; remove all `JointQuantity` / `PropagationDirection` references.
- Create new `test/joint_map/` directory (does not exist yet — current test tree only has `test/forward/eigen/`, `test/collision/fcl/`, `test/main.cpp`). Register it in CMake.
- New `test/joint_map/test_transmission_subgraph.cpp` (no FK dependency) — unit tests for the subgraph against synthetic `TransmissionAnalysis` instances.
- New `test/joint_map/test_missing_input_resolution.cpp` — unit tests for the resolution helper.

---

## Suggested execution order

1. **TransmissionAnalysis cleanup.** Revert `AffineTransmission` to `JointId` form. Add `AffineProjectionRule` struct + the `InterfaceId → AffineProjectionRule` registry on `TransmissionAnalysis` (with set/query API). Confirm `transmission_analysis.hpp` knows nothing about URDF / mimic / ros2_control.
2. **Delete legacy plan scaffolding.** Delete `transmission_plan.hpp/.cpp` and `transmission_analysis_joint_map_builder.hpp/.cpp`. Delete the `compile_*_plan_expected` APIs from `transmission_joint_map` and `composite_joint_map`. Strip every remaining reference to `JointQuantity` / `PropagationDirection` from headers and impls. Update CMakeLists.txt to remove the deleted source files. The codebase is intentionally non-compiling between this step and step 6; that's fine.
3. **Define `JointMapBuilder` shape.** Finalize the canonical `build_expected(span<const StateInterfaceId>, span<const StateInterfaceId>)` signature. Define `JointMapBuildError` and the `MissingInputResolution` helper header (impl can stub for now).
4. **Build out `TransmissionSubgraph`.** Fields, invariants, algorithm, queries. The subgraph reads `AffineProjectionRule`s from analysis when projecting affine relationships per interface. Add focused unit tests in `test/joint_map/test_transmission_subgraph.cpp` against synthetic `TransmissionAnalysis` instances — no FK dependency.
5. **Reconnect runtime joint maps.** Give `TransmissionJointMap` and `CompositeJointMap` direct constructors / factories the builder can call without any plan struct intermediary.
6. **Rewire `DefaultJointMapBuilder`.** Construct a `TransmissionSubgraph`, fail loudly with rich error on ambiguous or incomplete plans (using `compute_missing_input_resolutions`), and otherwise emit a `JointMap` directly by walking the subgraph (`producer_of()` per output, `selected_transmissions()` in topological order for staging). Codebase compiles again from this step on.
7. **Restructure ros2_control wrappers.** `Ros2ControlPluginTransmissionModel` loads its plugin once at import; `Compute` holds preallocated handles. Move mimic import to use joint-level `add_affine_transmission`. The default position/velocity/effort/acceleration projection rules are already populated by `TransmissionAnalysis`'s constructor — the importer does not need to register them.
8. **Test split + updates.** Create `test/joint_map/` directory. Split `test_eigen_fk_mapper.cpp` into focused files. Update all call sites to the new API. Add `test_transmission_subgraph.cpp` and `test_missing_input_resolution.cpp`. Update CMakeLists.txt.
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
    - viability filtering: a transmission with unreachable inputs is **not** counted as a candidate, even if its outputs are needed
    - missing-inputs reporting when outputs are unreachable
    - `add_input` unblocks a previously-incomplete plan
    - `add_input` resolves a previously-ambiguous interface
    - mixed-quantity transmission (position-in / effort-out)
    - ambiguity accumulation: multiple ambiguous interfaces in one pass are all reported
    - non-ambiguous interfaces in an otherwise-ambiguous plan still return their unique producer
  - `TransmissionAnalysis` affine group tests:
    - isolated joint → `affine_root_of(j) == j`, `affine_group_members(j)` is `[j]`
    - chain of mimics A → B → C → all share the same root; members list contains all three
    - `multiplier == 0` is rejected by `add_affine_transmission`
  - `compute_missing_input_resolutions` unit tests:
    - empty missing list → empty resolutions
    - single missing interface with one producing transmission → one transmission alternative
    - single missing interface with multiple producing transmissions → multiple transmission alternatives
    - missing interface in a non-trivial affine group with a registered rule → `affine_root` populated
    - missing interface in a trivial affine group → `affine_root == nullopt`
    - missing interface whose interface id has no projection rule → no affine resolution offered
    - already-supplied alternatives are filtered out
  - `DefaultJointMapBuilder` end-to-end:
    - returns rich `JointMapBuildError` with `MissingInputs` when inputs are missing, including resolution hints
    - returns rich `JointMapBuildError` with `Ambiguous` when multiple viable producers exist
    - successfully emits an `AffineJointMap` for a pure-affine request
    - successfully emits a `CompositeJointMap` for a mixed affine + transmission request

Existing tests are split out of `test_eigen_fk_mapper.cpp` into focused files as part of this work.

---

## Open items / out of scope

- `NamedStateInterfaceDefinition` convenience overloads on `JointMapBuilder` (deferred — canonical `StateInterfaceId` API only for now).
- Default-value-supplying joint map builder strategies (the seam exists; no concrete implementation).
- ros2_control controller helpers for sourcing missing state interfaces (acknowledged use case; future work).
- Ambiguity *resolution* policies (e.g. "prefer transmission X over Y when both are valid"). The current design always reports ambiguity rather than picking a winner — the user must remove the ambiguity from their setup. Smarter resolution can be added later as a builder-level concern, not a subgraph-level one.

