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

### `AffineTransmission` — joint-level, stored per-joint as a flat relation
The current state-interface form is a step in the wrong direction. A mimic relationship is intrinsically a joint-to-joint invariant (one joint mirrors another), and applies uniformly across whatever interfaces (position, velocity, …) the planner happens to need — but the *projection rule* depends on the interface type.

```cpp
struct AffineTransmission {
  JointId source_joint_id;   // by invariant: == affine_parent_[target_joint_id]
  JointId target_joint_id;
  float multiplier;          // composed flat from source to target — see below
  float offset;
};
```

**Storage shape (the key insight):** `affine_transmissions_` is **indexed by `JointId`** — one entry per joint, accessed via `affine_transmission_of(j)`. Each entry is the **already-composed flat relation** of that joint to its current affine-group root:

- `target_joint_id == j` (the joint this entry is for)
- `source_joint_id == affine_parent_[j]` (the joint's current root)
- `joint_value(j) = multiplier · joint_value(source_joint_id) + offset`
- For root joints, the entry is the identity `(source=self, m=1, o=0)`.

`add_affine_transmission(source, target, m, o)` composes the new edge into this flat form **eagerly at the time of the call**. It walks the affected group's member list once, updating every member's stored relation to be expressed in terms of the new (winning) root. This means:

1. The planner reads the (m, o) between any two joints in the same affine group as an O(1) lookup of two flat per-joint relations and a tiny algebraic step — no chain walking, no recursion, no per-query composition.
2. The planner never has to do the recursive `find_source`-style chain composition that the old `AffineJointMap` did at runtime.
3. Reads are pure (no path compression, no mutation), so the analysis is safe for concurrent reads.
4. **Affine compute is fundamentally SIMD-friendly.** At runtime an affine joint map is the inner loop `output[i] = input[sources[i]] * multipliers[i] + offsets[i]` — a tight, vectorizable kernel that `AffineJointMap::map()` already drives through `#pragma omp simd`. Builders are required to consolidate all affine producers within a `CompositeJointMap` stage into a single `AffineJointMap` segment so this kernel actually runs over many outputs at once, rather than degenerating into one tiny one-output affine compute step per joint. See the "Affine batching is a hard requirement" note under `DefaultJointMapBuilder` behavior.

**Precondition: `multiplier != 0`**, validated on `add_affine_transmission`. Allowing `multiplier == 0` would model `target = offset` (a constant value with no actual link to `source`) — that isn't really a mimic relationship and doesn't belong in `AffineTransmission`. Joints that are always at a constant value are handled by a different mechanism (the future default-value-source seam — a JointMapBuilder strategy listed in "Open items" that supplies fallback values for missing inputs, or the caller supplies the value as an input directly). With `multiplier != 0`, every affine relationship is guaranteed bidirectional, which keeps the affine-group semantics clean (see next).

**Precondition: `source_joint_id != target_joint_id`**, validated on `add_affine_transmission`. Self-loops are degenerate and always a user error.

**Composition arithmetic.** When `add_affine_transmission(source, target, m, o)` is called:
- `T_source = affine_transmission_of(source)` and `T_target = affine_transmission_of(target)` are the existing per-joint flats
- The new edge says `target = m · source + o`. Substituting source's flat (`source = T_source.m · R_s + T_source.o`):
  - `target_new = (m · T_source.m) · R_s + (m · T_source.o + o)` from the winner root `R_s = T_source.source_joint_id`
- Equating with the old `target = T_target.m · R_t + T_target.o` gives the loser-to-winner transformation:
  - `R_t = (target_new_m / T_target.m) · R_s + ((target_new_o − T_target.o) / T_target.m)`
- Every member j of the loser's old group has its stored relation rewritten by substituting this transformation: `m_j_new = m_j · M`, `o_j_new = m_j · O + o_j`, source = winner.
- The redundant case (winner root == loser root) is a no-op, debug-asserted to be numerically consistent within tolerance with the existing entry.

`TransmissionSubgraph` is responsible for projecting these joint-level flat relations onto the specific interface(s) required by a given build request, using the projection rules described next.

### `AffineProjectionRule` (new) and the projection registry on `TransmissionAnalysis`
Different interfaces project an affine joint-level relationship differently:
- **Position**: `q_target = m * q_source + o` — direct
- **Velocity**: `v_target = m * v_source` — offset drops under d/dt
- **Effort** (energy-conserving coupling, opt-in): `τ_source = m * τ_target` — same magnitude factor, but the relationship runs target → source (knowing the load torque tells you the actuator torque)

A rule encodes how to derive the projected affine relationship for a given interface from the underlying joint-level `AffineTransmission`'s `(multiplier, offset)`:

```cpp
struct AffineProjectionRule {
  // Effective relationship:
  //   projected_target_iface = effective_m * projected_source_iface + effective_o
  // where:
  //   effective_m = multiplier * multiplier_scale
  //   effective_o = offset * offset_scale
  // If reverse_direction is true, the source/target roles swap before applying
  // (i.e. projected_source_iface = AffineTransmission::target_joint_id's interface,
  // projected_target_iface = AffineTransmission::source_joint_id's interface).
  float multiplier_scale = 1.0f;
  float offset_scale = 1.0f;
  bool reverse_direction = false;
};
```

`TransmissionAnalysis` owns an `InterfaceId`-keyed map of these rules and **populates default entries on construction** for position, velocity, and acceleration. Callers can override or add more via `set_affine_projection_rule(InterfaceId, AffineProjectionRule)`.

Built-in defaults:

| InterfaceId | `multiplier_scale` | `offset_scale` | `reverse_direction` | Effective relation |
|---|---|---|---|---|
| `"position"` | 1.0 | 1.0 | false | `q_b = m·q_a + o` |
| `"velocity"` | 1.0 | 0.0 | false | `v_b = m·v_a` |
| `"acceleration"` | 1.0 | 0.0 | false | `a_b = m·a_a` |

**Effort is intentionally not registered by default.** URDF mimic joints are kinematic constraints — typically enforced by control logic rather than physical gears or belts — so energy-conserving effort propagation is a strong assumption that could silently produce wrong joint torques. Users who actually have a physical coupling and want effort to propagate must opt in explicitly:

```cpp
analysis.set_affine_projection_rule(
  InterfaceId{"effort"},
  AffineProjectionRule{
    .multiplier_scale = 1.0f,
    .offset_scale = 0.0f,
    .reverse_direction = true,  // τ_source = m·τ_target via energy conservation
  });
```

An interface with no registered rule is treated as "affine does not propagate" — the subgraph will not use mimic relationships to satisfy it, and any unreachable output that could only be reached via affine projection through that interface will surface as unreachable.

### `TransmissionModel` (already correct — keep)
```cpp
virtual unique_ptr<const ComputeTransmission> build(
  span<const StateInterfaceId> input_ids,
  span<const StateInterfaceId> output_ids) const = 0;
```
No quantity, no direction. The model decides what to compute purely from which interfaces are listed in inputs vs outputs.

**Contract: `build(input_ids, output_ids)` is only ever called with `(input_ids, output_ids)` matching a `TransmissionInstance` previously registered for this model.** Implementers can `assert` and assume validity. The builder enforces this by selecting `TransmissionInstance`s from the analysis and passing their exact `input_ids`/`output_ids` to the corresponding model's `build()`. Custom (non-ros2_control) `TransmissionModel` implementations should rely on this contract — there is no need for `build()` to return failure for "unsupported combinations" because the caller is structurally prevented from asking.

### `ComputeTransmission` (already correct — keep)
Unchanged: `compute(inputs, outputs, scratch)`.

### `TransmissionAnalysis` (cleanup)
- **Knows nothing about URDF, ros2_control, or mimic joints** — purely a typed graph data structure.
- Holds: joint order, state-interface order, models, transmissions (state-interface-edged), per-joint flat affine relations (`affine_transmissions_` indexed by `JointId`, see the `AffineTransmission` section above), the `InterfaceId → AffineProjectionRule` registry as `std::unordered_map<InterfaceId, AffineProjectionRule>` (using `InterfaceId`'s existing FNV1a hash; `Order<>` is the wrong tool here — no stable internal IDs are needed), and the **affine group** index (see below).
- Maintains an **inverse index** from `StateInterfaceId → vector<TransmissionInstanceId>` of transmissions whose `output_ids` contain that interface. Built incrementally as `add_transmission` is called. Lets the subgraph algorithm answer "who produces X?" in expected O(1) instead of O(num_transmissions × num_outputs).
- API:
  - `add_affine_transmission(JointId source, JointId target, float m, float o)` — joint-level edge. Composes into the per-joint flat relations and updates the affine group index.
  - `affine_transmission_of(JointId) const → const AffineTransmission &` — O(1) lookup of a joint's current flat relation to its root.
  - `affine_root_of(JointId) const → JointId` — O(1) lookup of a joint's current root.
  - `affine_group_members(JointId root) const → span<const JointId>` — members of an affine group, by root.
  - `set_affine_projection_rule(InterfaceId, AffineProjectionRule)` and `affine_projection_rule(InterfaceId) const → const AffineProjectionRule *` (returns `nullptr` if no rule registered).
- `TransmissionAnalysis` is **append-only** — there is no remove API for transmissions, affine transmissions, models, or projection rules. The inverse transmission index and the eagerly-flattened affine group index can therefore be maintained incrementally without ever needing to handle removals.
- **Duplicate output state interfaces are allowed** in `TransmissionInstance::output_ids` (and in builder requests). Asking for the same value in multiple places is a legitimate use case and is not an error.

### Affine group queries (root-joint identification)
Connected components of the joint graph induced by `add_affine_transmission` calls are first-class. Because `multiplier != 0` is enforced, every relation is bidirectional and each component has the property that **defining any single joint in the group defines all of them** (the (m, o) between any two joints in the group is computed in O(1) from their per-joint flat relations to the shared root).

Groups are identified by their **root joint**, which is itself a valid `JointId` and can be named in error messages. Every joint is in a group (possibly a trivial group of one, where the root is the joint itself), so no sentinel id is needed. The "root" has a directional meaning: it is the source-side joint at the end of the affine chain, the joint everything else ultimately derives from. `add_affine_transmission(source, target, …)` always makes `source`'s current root win the merge.

```cpp
// On TransmissionAnalysis:
// Returns the root joint of the affine group containing j (O(1), single array lookup).
// If j has no affine relationships, returns j itself.
[[nodiscard]] JointId affine_root_of(JointId j) const noexcept;
// All members of the group whose root is `root`. Always contains at least `root`.
[[nodiscard]] span<const JointId> affine_group_members(JointId root) const noexcept;
// Returns the per-joint flat relation: joint_value(j) = m · joint_value(root) + o.
// Root joints have the identity entry (m=1, o=0). O(1) lookup.
[[nodiscard]] const AffineTransmission & affine_transmission_of(JointId j) const noexcept;
```

The index is maintained eagerly as `add_affine_transmission` is called — each new edge composes into the per-joint flat relations and updates every affected member's stored entry in a single pass. The tree is kept maximally flat: every joint's parent always points directly at its current root, so `affine_root_of` is a single array lookup with no walking and no path compression. This makes both the subgraph algorithm and the missing-input resolution helper much simpler than walking edges on demand — they just read the already-composed flat (m, o) from `affine_transmission_of`.

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
- Cached list of **unreachable outputs** — needed interfaces with no viable producer

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
    StateInterfaceId source;   // the known leaf interface providing the value (always
                               // resolves to producers::Input or producers::Transmission
                               // in one producer_of() step — never another AffineProjection)
    float multiplier;          // already-composed interface-space coefficient from source to
                               // this output, computed in O(1) at planning time from the
                               // analysis's per-joint flat affine relations and the projection
                               // rule for this interface id
    float offset;
  };
  struct Transmission {
    TransmissionInstanceId transmission;
  };
}
// std::monostate represents "not produced" (interface is unreachable, ambiguous,
// or out-of-scope — see producer_of() doc below).
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
  // The plan is fully usable iff is_complete() returns true.
  bool is_complete() const noexcept;     // !is_ambiguous() AND unreachable_outputs() is empty
  bool is_ambiguous() const noexcept;    // any interface in the plan has multiple viable producers

  // The original inputs span passed to the constructor, plus anything appended
  // via add_input(), in insertion order. This is the "what the user supplied"
  // view; positions in this span correspond to producers::Input::input_index.
  span<const StateInterfaceId> requested_inputs() const noexcept;
  // The full set of derivable interfaces — user inputs plus everything reachable
  // through transmissions and affine projections. Use this when you want to know
  // "is X part of the working set?" rather than "did the user supply X?"
  span<const StateInterfaceId> derivable_interfaces() const noexcept;

  // Needed outputs that the algorithm could not derive from the requested inputs.
  // Empty when the plan is complete.
  span<const StateInterfaceId> unreachable_outputs() const noexcept;

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

  // The principal builder query: how is this state interface produced in the plan?
  // O(1) lookup. Returns std::monostate if the interface is unreachable, ambiguous,
  // or out-of-scope (not in derivable_interfaces() — i.e. an interface from the
  // analysis that the algorithm never even considered for this request). Builders
  // should check is_ambiguous() before walking outputs.
  StateInterfaceProducer producer_of(StateInterfaceId) const noexcept;

  // Each entry: an interface that was added via add_input() and which had
  // pre-existing non-Input candidate producers that the override discarded.
  // Builders can surface these as warnings ("you supplied X explicitly, but the
  // analysis says X is also derivable via transmission Y — using your value").
  // Only populated when add_input() actually overrode something; empty for
  // initial_inputs (which are exclusive without warning, since they were
  // declared at construction time).
  //
  // Note: a discarded Transmission candidate in this list does NOT mean the
  // transmission is unselected entirely. It only means the transmission is no
  // longer the chosen producer FOR THIS interface. The same transmission may
  // still be selected (and appear in selected_transmissions()) because it
  // produces other needed outputs.
  struct InputOverride {
    StateInterfaceId interface;
    std::vector<StateInterfaceProducer> discarded_candidates;
  };
  span<const InputOverride> input_overrides() const noexcept;
};
```

**Builder note on AffineProjection source.** A `producers::AffineProjection` carries a `source` `StateInterfaceId` that is **guaranteed to resolve to a leaf in one step** — either `producers::Input` or `producers::Transmission`. The algorithm enforces this by always picking a known interface whose producer is already a leaf when emitting an affine projection candidate. There is no chain to walk; `producer_of(source)` always returns a leaf, never another `AffineProjection`. The `multiplier` and `offset` carried in the `AffineProjection` are the already-composed interface-space coefficients from the leaf source directly to the projected interface, made cheap by the per-joint flat storage in `TransmissionAnalysis::affine_transmissions_`.

The subgraph deliberately does **not** expose an "ordered execution stages" method. Builders are responsible for emitting their own staging by walking the subgraph through these queries — the subgraph is a pure analysis utility, not a planning pipeline.

Algorithm execution timing (eager-in-constructor vs lazy-on-first-query) is an **implementation detail**. The plan does not prescribe one — pick whichever is most efficient at implementation time. The strong invariants below must hold immediately after any mutation regardless.

### Invariants
- Never holds dangling references — all `JointId`/`StateInterfaceId`/`TransmissionInstanceId` values exist in `analysis_`.
- Selected transmissions form a DAG (no cycles).
- `selected_transmissions()` is returned in topological order — dependencies before dependents.
- **Ambiguity is accumulated and surfaced via a query, never silently resolved.** The algorithm continues past ambiguous interfaces, marking each one with its competing candidates, and exposes the full set via `ambiguous_interfaces()`. `is_complete()` requires both `!is_ambiguous()` and an empty `unreachable_outputs()`.
- For ambiguous interfaces, `producer_of()` returns `std::monostate`. Non-ambiguous interfaces in the same plan still return their unique producer.
- **`Input` producers are exclusive.** Any interface that the user supplied (either in `initial_inputs` or via `add_input`) has exactly one producer: `producers::Input{...}`. The algorithm never records competing `Transmission` or `AffineProjection` candidates for an interface that already has an `Input` producer. This honors the user's explicit declaration of authority over a value — providing it as input means "use this exact value, don't try to derive it." Non-Input ambiguity (Transmission vs AffineProjection candidates competing for the same non-user-supplied interface) is still reported as ambiguous; only `Input` wins implicitly.
- `derivable_interfaces() ∩ unreachable_outputs() == ∅`. If an interface appears in both `initial_inputs` and `initial_outputs`, it is treated as known (produced by `producers::Input`) and never appears in `unreachable_outputs()`.
- `add_input(I)` always wins over any pre-existing producer for `I` **on its first occurrence**. If `I` is not yet in the effective input list and the snapshot of `producer_of(I)` taken before the call held an `AffineProjection` or `Transmission` producer (or was ambiguous between several), those candidates are discarded and `I`'s producer becomes `producers::Input{...}`; the discarded candidates are appended to the input-override log.
  - On subsequent `add_input(I)` calls (i.e. when `I` is already in the effective input list), the call appends a duplicate entry to `requested_inputs()` but does not change `producer_of(I)` and does not append to the override log. This matches the "duplicates allowed" semantics of `initial_inputs`.
  - Because `add_input` after construction is a less obviously-intentional act than passing the interface in the constructor, the subgraph records each first-occurrence override in an "input override" log accessible via a query (see below). Builders can surface these as warnings to the user.

### Algorithm sketch
The algorithm is a **forward fixed-point** over viable producers, not a naive backward walk. A transmission is only a *candidate* producer for one of its outputs once **all of its inputs are themselves derivable** — otherwise it cannot run, so it cannot disambiguate or block other paths.

The algorithm runs in two distinct phases: first compute the full reachable set (no candidate counting), then in a single pass over the converged set, count candidates per interface and classify each one. Doing the counting only after convergence is essential — an interface that has 1 candidate at iteration N can gain a 2nd candidate at iteration N+5 once another transmission becomes viable, so any in-loop ambiguity verdict would be wrong.

1. **Initialize.** `known = initial_inputs`. `needed = initial_outputs \ initial_inputs`. Walk `initial_inputs` in order; for each interface, if it does not yet have a producer assigned, record `producers::Input{input_index}` where `input_index` is its position in the span. **First occurrence wins** — if the same interface appears twice in `initial_inputs`, the second occurrence is ignored for producer assignment (but both positions exist in `requested_inputs()`). Interfaces that appear in both `initial_inputs` and `initial_outputs` are therefore handled correctly: they go into `known` with their `Input` producer and never enter `needed`.
2. **Compute reachability fixed point.** Iterate until no new interfaces are added to `known`:
   - For every `TransmissionInstance T` in `analysis_.transmissions()`: if every interface in `T.input_ids` is in `known`, then every interface in `T.output_ids` becomes derivable. For each output interface, **if it does NOT already have an `Input` producer, record `T` as one of its candidate producers.** (Input producers are exclusive — see Invariants.)
   - For every joint `J` whose `(J, I)` is in `known` and whose `I` has a registered `AffineProjectionRule`: every other joint `J'` in `analysis.affine_group_members(analysis.affine_root_of(J))` has `(J', I)` derivable via affine projection. The interface-space `(multiplier, offset)` from `(J, I)` to `(J', I)` is computed in **O(1)** by reading both joints' pre-composed flat relations from the analysis and a single algebraic step:
     - `T_J = analysis.affine_transmission_of(J)` — flat: `J = T_J.m · root + T_J.o`
     - `T_J' = analysis.affine_transmission_of(J')` — flat: `J' = T_J'.m · root + T_J'.o`
     - Joint-space relation `J' = m_joint · J + o_joint` where `m_joint = T_J'.m / T_J.m` and `o_joint = T_J'.o − m_joint · T_J.o`
     - Then apply the projection rule for `I` (`multiplier_scale`, `offset_scale`, `reverse_direction` source/target swap) to get the interface-space `(m, o)` the planner records.
     - No chain walking, no recursion, no per-query composition — `add_affine_transmission` did all the chain composition eagerly when the edges were added.
   - When picking the affine source for `(J', I)`, prefer a known interface whose producer is **already a leaf** (`Input` or `Transmission`), so the resulting `producers::AffineProjection` always sources directly from a leaf. This is straightforward: when iterating known interfaces in the group, take the first one that resolves to a leaf via `producer_of()`.
   - **If `(J', I)` does not already have an `Input` producer**, record this affine projection as a candidate producer for it.
   - Add newly-derivable interfaces to `known`. Note: a newly-derivable interface is added to `known` regardless of whether it has 1 or more candidate producers — its "knownness" propagates downstream so the full reach is still computed.
3. **Classify each interface in the converged candidate set.** A single pass:
   - 0 candidates → not produced (will only matter if it's a needed output).
   - 1 candidate → record that single producer in the producer-assignment map (backs `producer_of()`).
   - ≥ 2 candidates → record as ambiguous, with the full candidate list, in the ambiguous set (backs `ambiguous_interfaces()`).
4. **Classify needed outputs.**
   - In `known` and unambiguous → `producer_of()` returns the recorded producer; the output is satisfied.
   - In `known` and ambiguous → contributes to `ambiguous_interfaces()`; `producer_of()` returns `monostate`.
   - Not in `known` → contributes to `unreachable_outputs()`.
5. **Topological emission.** `selected_transmissions()` is built by tracing back from every needed output that has a unique `Transmission` or `AffineProjection` producer (recursively, via `producer_of(source)` for affine chains), collecting every `TransmissionInstance` encountered. This set is then sorted such that `T1` precedes `T2` whenever any input of `T2` is produced by `T1`. The dependency graph is a DAG by construction — every transmission's inputs are derivable from a strict subset of `known` at the time it first became viable.
6. **Mutation.** `add_input(I)` always appends `I` to the subgraph's effective input list (with the new index `effective_inputs.size()` before insertion). The producer assignment then follows "first occurrence wins":
   - **If `I` was not previously in the effective input list**: take a snapshot of `producer_of(I)` *before* the change. Forcibly assign `producer_of(I) = producers::Input{new_index}`. If the snapshot held one or more non-Input candidates (a single Transmission/AffineProjection or an ambiguous set of them), append `(I, snapshot_candidates)` to the input-override log accessible via `input_overrides()`.
   - **If `I` was already in the effective input list** (whether via `initial_inputs` or a previous `add_input`): the duplicate entry exists in `requested_inputs()` but `producer_of(I)` is unchanged (still points at the original `Input{first_index}`). No override log entry is added. This matches the "duplicates allowed" semantics of `initial_inputs`.
   - In either case, the fixed point then re-runs from step 2. Newly-viable transmissions may now produce other interfaces, and previously-ambiguous interfaces that depended on `I`'s old non-Input producer may resolve.

   `add_output(I)` adds `I` to `needed` and re-classifies.

This formulation has the nice property that "viability" is a fixed point — we never have to undo a candidate decision because of input unreachability. Ambiguity is detected accurately (only between *actually viable* paths) and is accumulated across the entire pass.

**Side-effect outputs of selected transmissions.** A `TransmissionInstance` is a block: it produces *all* of its `output_ids` in a single `compute()` call, not on a per-output basis. So if transmission `T` is selected because the user asked for output `K` (one of T's outputs), and `T`'s other outputs include some interface `I` that the user supplied directly as input, the runtime will still execute `T` and `T` will compute a value for `I`. That computed value is silently discarded — `producer_of(I) == Input{...}` means the JointMap writes the user's `I` value into the output position, not `T`'s side-effect value. This is semantically correct (the user's authority over `I` wins) and unavoidable given block-transmission semantics; it is not a missed optimization to flag.

**`add_input` and `producers::Input::input_index`.** The subgraph maintains an effective input list that starts as a copy of the constructor's `initial_inputs` span and grows by one each time `add_input` is called. The `input_index` field of `producers::Input` is the position in this effective list of the interface's **first occurrence** — so original (unique) inputs use indices `[0, initial_inputs.size())` and freshly-added inputs use `initial_inputs.size()` and beyond, in the order they were added. Duplicate entries (whether from a repeated interface in `initial_inputs` or from `add_input(I)` where `I` is already present) occupy their own slot in the effective list but do *not* change `producer_of(I).input_index` — it always names the first occurrence. Builders that intend to materialize a `JointMap` from a subgraph that has been mutated must be aware that input indices past the original span size refer to interfaces the builder wasn't told about at request time; in practice this means most builders should not call `add_input` on a subgraph they intend to materialize — `add_input` is most useful for analysis tools, tests, and builder strategies that know how to source the extra values themselves.

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
  };
  Kind kind = Kind::MissingInputs;
  // Human-readable message. Should reference TransmissionInstance::name when
  // identifying transmissions in ambiguity reports or resolution hints, so users
  // see "transmission `differential_left`" rather than "transmission 3".
  std::string message;

  // Populated when kind == MissingInputs.
  std::vector<StateInterfaceId> unreachable_outputs;
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
- Use `subgraph.requested_inputs()` to filter out trivially-already-supplied alternatives.
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
- If `subgraph.is_ambiguous()`: return failure with `kind = Ambiguous` and `ambiguous_interfaces` populated from `subgraph.ambiguous_interfaces()`. The message names each ambiguous interface and its candidate producers. **Ambiguity wins over unreachability**: if both occur in the same plan, the user fixes the ambiguity first and retries, then sees the unreachable-outputs error on the second attempt. The `unreachable_outputs` field is left empty in the `Ambiguous` case.
- If `!subgraph.is_complete()` (and not ambiguous): return failure with `kind = MissingInputs`, `unreachable_outputs` populated from `subgraph.unreachable_outputs()`, and resolution hints from `compute_missing_input_resolutions(subgraph)`.
- Otherwise: walk the subgraph (using `producer_of()` for each requested output, plus `selected_transmissions()` in topological order for staging) and emit a `JointMap` directly. For `producers::AffineProjection`, call `producer_of(source)` once to find the underlying data location — by invariant the result is always a leaf (`Input` or `Transmission`), no further recursion needed. The `(multiplier, offset)` carried in the `AffineProjection` is the final interface-space coefficient and can be used directly. The builder constructs the appropriate concrete runtime type:
  - Pure-affine plan (no transmissions selected) → a single `AffineJointMap`.
  - Single transmission, no affine stages → a transmission-backed joint map directly.
  - Mixed → `CompositeJointMap` over affine + transmission segments.
  - The builder is free to extract sub-helpers as needed; this is normal code, not a single inline function.

**Affine batching is a hard requirement, not an optimization.** Affine producers are essentially `output[i] = m[i] * input[src[i]] + o[i]` — a tight, vectorizable inner loop, exactly what `AffineJointMap::map()` already runs through `#pragma omp simd`. Whenever the builder emits a `CompositeJointMap`, it must **batch every output that is sourced from a `producers::Input` or `producers::AffineProjection` (with a leaf source) into a single `AffineJointMap` segment**, rather than producing one tiny one-element affine compute step per output. The same goes for outputs that are direct input pass-throughs (`producers::Input`) — they fold into the same `AffineJointMap` as identity rows (`m=1`, `o=0`, `src=input_index`).

Concretely: while walking the subgraph's outputs in order, the builder collects three parallel arrays — `sources[]` (input index of the leaf data), `multipliers[]`, `offsets[]` — for every output whose producer is `Input` or `AffineProjection`. Outputs whose producer is `Transmission` (a block compute that produces multiple values together) interrupt the collection: they emit a `TransmissionJointMap` segment, then the builder resumes collecting affine outputs into a fresh `AffineJointMap` segment after the transmission. The end state is a `CompositeJointMap` of alternating affine and transmission segments, where every affine segment is one contiguous SIMD-friendly compute that handles as many outputs as possible in a single `map()` call. A degenerate case where the entire request is affine collapses to a single `AffineJointMap` directly (no `CompositeJointMap` wrapper needed).

Error messages should report exactly what is missing, and what would need to be supplied to resolve the issue (if multiple possible resolutions exist, this should be clearly communicated in error messaging).

In a correctly-configured robot, missing inputs only happen when the user has forgotten to expose required state interfaces in their ros2_control setup — it is a configuration error, not a recoverable runtime case. The builder fails loudly so the user can see exactly what they need to fix. A future addition will be controller-side helpers that automatically derive the required state interface set from a `JointMap`'s declared inputs and request them through the controller's interface configuration, so users never have to manually figure out the requirements by reading errors and editing config.

### Custom builders (future)
FK plugins can subclass `JointMapBuilder` to provide alternative behaviors. The two anticipated extensions, neither of which is in scope for this refactor:
- A builder that supplies **default values** for missing inputs (either directly defaulting unreachable outputs to a constant, or sourcing default values for inputs identified by `compute_missing_input_resolutions` so that the upstream transmissions become viable) via a small "default value source" interface.
- A builder that emits a richer error report tailored to a specific FK plugin's domain.

The seam is clean: subclass `JointMapBuilder`, hold a reference to the analysis, build a `TransmissionSubgraph`, and react to its queries however the strategy requires.

---

## Plan / Runtime Types

The legacy `make_*_plan_expected` helper functions and the intermediate `JointMapPlan`/`AffinePlan`/`TransmissionPlan` plan structs are scaffolding from the previous (joint+quantity+direction) design. They are **deleted entirely**. The builder constructs concrete `JointMap` runtime types directly from the subgraph queries — extracting helpers as needed. Don't impose "everything inline in one function" — use judgment.

**File:** `include/arm_kinematics/joint_map/transmission_plan.hpp` + cpp — **DELETE**.

**Files:** `include/arm_kinematics/joint_map/transmission_joint_map.hpp` + cpp
- Remove the `compile_transmission_plan_expected` API entirely.
- Replace with a constructor (or factory) on `TransmissionJointMap` that takes the **runtime/compute** objects directly: a sequence of `unique_ptr<const ComputeTransmission>` instances along with their input/output index mappings into the JointMap's overall input/output spans. The builder produces these by walking the subgraph and calling `TransmissionModel::build(input_state_interface_ids, output_state_interface_ids)` on each selected transmission's model — that call returns the appropriate `ComputeTransmission`, which the JointMap then owns. After construction, the JointMap holds no references back to `TransmissionAnalysis` / `TransmissionModel` / `TransmissionInstance` — it touches only compute-side objects.

**Files:** `include/arm_kinematics/joint_map/composite_joint_map.hpp` + cpp
- Remove the `compile_joint_map_plan_expected` API entirely.
- `CompositeJointMap` keeps its existing run-time shape (a sequence of segment joint maps over output index ranges), but is now constructed directly from the segments the builder emits — no intermediate plan struct.
- Note: by the affine-batching rule (see `DefaultJointMapBuilder` behavior), every affine segment that the builder hands to `CompositeJointMap` is one consolidated `AffineJointMap` covering as many outputs as possible — never one segment per output. `CompositeJointMap` itself doesn't enforce this; it just handles whatever segment list it's given. The batching is the builder's responsibility.

**Files:** `include/arm_kinematics/joint_map/affine_joint_map.hpp` + cpp
- No structural change. Verify it still compiles after the joint-level `AffineTransmission` revert. The builder constructs `AffineJointMap` directly from `(sources[], multipliers[], offsets[])` arrays it computes from the subgraph in a single pass.
- The runtime `map()` is the SIMD-friendly hot loop (`#pragma omp simd` over `output[i] = input[sources[i]] * multipliers[i] + offsets[i]`) — keeping the affine compute consolidated into one `AffineJointMap` per stage is essential for the planner to actually exploit this. See the affine batching note in the `DefaultJointMapBuilder` behavior section.

---

## ros2_control wrapper

**File:** `src/joint_map/transmission_analysis_import.cpp` (+ corresponding headers)

Critical principle (from the user): `TransmissionAnalysis` must never know about ros2_control. The wrappers go through the **same** `TransmissionModel` / `ComputeTransmission` mechanisms as any other transmission. No special path.

### `Ros2ControlPluginTransmissionModel`
- Implements `TransmissionModel`. Holds the metadata needed to instantiate the underlying `transmission_interface::Transmission` plugin (the loader handle, the parsed transmission XML, the joint/actuator role mapping, and a small bitset of supported `(direction × quantity)` combinations discovered at import time).
- Does **not** hold a long-lived `Transmission` instance. The plugin is instantiated once at import for capability probing (then discarded), and again per `build()` call to produce a fresh `ComputeTransmission`.
- `build(input_ids, output_ids)` instantiates a fresh ros2_control plugin via the loader, configures it for the specific `(direction, quantity)` combination implied by the input/output state interfaces, and wraps the configured `Transmission` (along with its handles) in a `Ros2ControlPluginTransmissionCompute`. The combination is guaranteed supported because the importer only registered `TransmissionInstance`s for combinations that passed probing — but the model can `assert` against its supported-bitset for safety.
- Drop all internal `JointQuantity` / `PropagationDirection` fields.

### `Ros2ControlPluginTransmissionCompute`
- Owns the freshly-instantiated `transmission_interface::Transmission` plugin and its preallocated handle storage. Lifetime is independent — once `build()` returns, this object is fully self-contained.
- `compute(inputs, outputs, scratch)`: copy inputs into handles, call the plugin, copy outputs out. Allocation-free.

### URDF importer (lives outside `TransmissionAnalysis`)
- Parses URDF and ros2_control transmission XML.
- For each ros2_control transmission, **probes supported combinations**: instantiate a throwaway `Transmission` plugin, allocate one-shot dummy storage (a small block of `double`s for actuator/joint values), and try to `configure()` the throwaway with dummy `ActuatorHandle`/`JointHandle`s pointing into that storage for each of the (direction × quantity) combinations: actuator→joint position, joint→actuator position, actuator→joint velocity, joint→actuator velocity. Combinations where `configure()` succeeds (no exception, no error return) are recorded as supported in a bitset on the `Ros2ControlPluginTransmissionModel`. The throwaway `Transmission` and its dummy storage are discarded immediately after probing — the model holds only the loader handle, the parsed XML, the role mapping, and the supported bitset.
- For each supported combination, register one `TransmissionInstance` in the `TransmissionAnalysis` pointing at the same `Ros2ControlPluginTransmissionModel` (one model per ros2_control transmission, shared across the up-to-4 instances).
- At joint-map build time, `Ros2ControlPluginTransmissionModel::build()` instantiates a *fresh* `Transmission` (loaded again via the loader), configures it for the requested combination, and hands ownership to the resulting `Ros2ControlPluginTransmissionCompute`. The plugin is therefore loaded twice: once at import for capability probing (then discarded), and once per `build()` call for actual compute use.
- If probing turns out to be expensive or fragile in practice, a fallback heuristic is to consult the plugin's class name against a small known-table; for unknown classes, optimistically register all 4 combinations and let `build()` failure surface the unsupported ones at JointMap construction time. The implementer should pick whichever proves more practical once they hit real plugin types.
- Mimic joints become `TransmissionAnalysis::add_affine_transmission(JointId, JointId, multiplier, offset)` calls. The importer is the only place that knows mimic joints exist. It does **not** need to register `AffineProjectionRule` entries for `"position"`/`"velocity"`/`"acceleration"` — `TransmissionAnalysis` populates those defaults on construction. The importer only registers projection rules when overriding a default or adding a custom interface id (e.g. opting into the `"effort"` rule for a robot whose mimic joints really do reflect a physical coupling).

---

## FK plugin and RobotModel call sites

The Eigen FK plugin currently consumes `JointMapBuilder` via `EigenForwardKinematicsPlugin::make_tree(joint_names, base_link_name, frames)`, where `joint_names` are plain strings. The new builder API takes `span<const StateInterfaceId>`. The FK plugin must therefore:

1. Hold a reference to (or get one from) the `TransmissionAnalysis` so it can resolve names → `StateInterfaceId`s.
2. Decide which `InterfaceId`(s) it wants to request. For the existing FK use case the plugin reads joint *positions* — so it requests `(joint_name, "position")` for each joint name. Velocity-aware FK paths (if any) would request `"velocity"` instead.
3. Call `analysis.ensure_state_interface_id(NamedStateInterfaceDefinition{name, InterfaceId{"position"}})` for each input/output joint name to get the `StateInterfaceId`s, then pass those spans to `builder.build_expected(...)`.
4. Handle the new `JointMapBuildError` (logging the message at minimum; ambiguity and unreachable-output errors during FK setup indicate a robot configuration bug).

`RobotModel::get_joint_map_builder()` continues to return a `JointMapBuilder &` (the new `DefaultJointMapBuilder` constructed against the robot's `TransmissionAnalysis`). No interface change at the `RobotModel` layer beyond the builder concrete type swapping under the hood.

**Files affected:**
- `src/plugins/forward/eigen_forward_kinematics_plugin.cpp` — `make_tree` resolves joint names to `(joint_name, "position")` `StateInterfaceId`s before calling the builder; handles the new error type.
- `include/arm_kinematics/forward/forward_kinematics_plugin.hpp` — `get_joint_map_builder()` virtual seam likely unchanged at the signature level (still returns `const JointMapBuilder &`); only the concrete returned object changes.
- `src/common/robot_model.cpp` (and corresponding header) — `get_joint_map_builder()` constructs and returns a `DefaultJointMapBuilder` over the robot's `TransmissionAnalysis`.

This is a real interface change for the FK plugin. It is intentionally in scope for this refactor (rather than deferred to a follow-up) so that the codebase compiles as a whole at the end.

**IK and collision plugins are not affected.** Verified by grepping for `JointMap`/`joint_map` references:
- `src/plugins/inverse/banksia_ik_plugin.cpp` only mentions `joint_map` in a comment (no actual code dependency).
- The collision plugin sources don't reference `JointMap` at all.
- `test/collision/fcl/test_fcl_collision_plugin.cpp` has unused `#include`s and `using` declarations for `JointMap`/`JointMapBuilder` that should be cleaned up — they don't actually exercise any joint map code, but the includes will pull in the new headers and the `using` declarations are dead.

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
- `include/arm_kinematics/joint_map/default_joint_map_builder.hpp/.cpp` — use `TransmissionSubgraph`; fail-loudly-with-rich-error strategy
- `src/joint_map/transmission_analysis_import.cpp` — drop `JointQuantity`/`PropagationDirection` internals; restructure `Ros2ControlPluginTransmission*` to load plugin once at import (try-configure probing for supported combinations) and dispatch from state interface ids; route mimic joints to joint-level `add_affine_transmission`
- `src/plugins/forward/eigen_forward_kinematics_plugin.cpp` — `make_tree` resolves joint names → `(name, "position")` `StateInterfaceId`s against the robot's `TransmissionAnalysis` before calling the builder; handles the new `JointMapBuildError`
- `src/common/robot_model.cpp` (+ header) — `get_joint_map_builder()` returns a `DefaultJointMapBuilder` constructed over the robot's `TransmissionAnalysis`
- `CMakeLists.txt` — remove deleted source files; add new `transmission_subgraph.cpp` and `missing_input_resolution.cpp`; register the new `test/joint_map/` test directory and the split eigen FK test files

**New:**
- `include/arm_kinematics/joint_map/missing_input_resolution.hpp` + cpp — reusable, self-contained `compute_missing_input_resolutions(const TransmissionSubgraph &)` helper used by builders to populate `JointMapBuildError::resolutions`.

**Tests:**
- Split `test/forward/eigen/test_eigen_fk_mapper.cpp` into focused files (e.g. `test_eigen_fk_reorder.cpp`, `test_eigen_fk_mimic.cpp`, `test_eigen_fk_transmission.cpp`). Update all sites to the new API; remove all `JointQuantity` / `PropagationDirection` references.
- Create new `test/joint_map/` directory (does not exist yet — current test tree only has `test/forward/eigen/`, `test/collision/fcl/`, `test/main.cpp`). Register it in CMake.
- New `test/joint_map/test_transmission_subgraph.cpp` (no FK dependency) — unit tests for the subgraph against synthetic `TransmissionAnalysis` instances.
- New `test/joint_map/test_missing_input_resolution.cpp` — unit tests for the resolution helper.
- `test/collision/fcl/test_fcl_collision_plugin.cpp` — remove the dead `#include`s for `joint_map.hpp` / `joint_map_builder.hpp` and the corresponding unused `using` declarations. The test does not exercise any joint map code; the includes are stale.

---

## Suggested execution order

1. **TransmissionAnalysis cleanup.** Revert `AffineTransmission` to `JointId` form. Add `AffineProjectionRule` struct + the `InterfaceId → AffineProjectionRule` registry on `TransmissionAnalysis` (with set/query API). Confirm `transmission_analysis.hpp` knows nothing about URDF / mimic / ros2_control.
2. **Delete legacy plan scaffolding.** Delete `transmission_plan.hpp/.cpp` and `transmission_analysis_joint_map_builder.hpp/.cpp`. Delete the `compile_*_plan_expected` APIs from `transmission_joint_map` and `composite_joint_map`. Strip every remaining reference to `JointQuantity` / `PropagationDirection` from headers and impls. Update CMakeLists.txt to remove the deleted source files. The codebase is intentionally non-compiling between this step and step 6; that's fine.
3. **Define `JointMapBuilder` shape.** Finalize the canonical `build_expected(span<const StateInterfaceId>, span<const StateInterfaceId>)` signature. Define `JointMapBuildError` and the `MissingInputResolution` helper header (impl can stub for now).
4. **Build out `TransmissionSubgraph`.** Fields, invariants, algorithm, queries. The subgraph reads `AffineProjectionRule`s from analysis when projecting affine relationships per interface. Add focused unit tests in `test/joint_map/test_transmission_subgraph.cpp` against synthetic `TransmissionAnalysis` instances — no FK dependency.
5. **Reconnect runtime joint maps.** Give `TransmissionJointMap` and `CompositeJointMap` direct constructors / factories the builder can call without any plan struct intermediary.
6. **Rewire `DefaultJointMapBuilder`.** Construct a `TransmissionSubgraph`, fail loudly with rich error on ambiguous or incomplete plans (using `compute_missing_input_resolutions`), and otherwise emit a `JointMap` directly by walking the subgraph (`producer_of()` per output, `selected_transmissions()` in topological order for staging). Production `joint_map` code compiles from this step; the FK plugin and tests are still broken until later steps.
7. **Restructure ros2_control wrappers.** `Ros2ControlPluginTransmissionModel` loads its plugin once at import (try-configure each combination with dummy handles to detect supported (direction × quantity) combos); `Compute` holds preallocated handles. Move mimic import to use joint-level `add_affine_transmission`. The default position/velocity/acceleration projection rules are already populated by `TransmissionAnalysis`'s constructor; the importer does not need to register them. (Effort is not a default — opt in if needed.)
8. **Update FK plugin and RobotModel call sites.** Modify `EigenForwardKinematicsPlugin::make_tree` to resolve joint names → `StateInterfaceId`s against the robot's `TransmissionAnalysis` (defaulting to the `"position"` interface), then call the new `build_expected(span<const StateInterfaceId>, ...)`. Handle the new `JointMapBuildError`. Update `RobotModel::get_joint_map_builder()` to return a `DefaultJointMapBuilder`. From this step the entire production codebase compiles again — only test files remain on the old API.
9. **Test split + updates.** Create `test/joint_map/` directory. Split `test_eigen_fk_mapper.cpp` into focused files. Update all call sites to the new API. Add `test_transmission_subgraph.cpp` and `test_missing_input_resolution.cpp`. Update CMakeLists.txt.
10. **Build, run all tests.**

---

## Verification

- `colcon build --packages-select arm_kinematics` succeeds.
- `colcon test --packages-select arm_kinematics` succeeds — all existing tests pass after migration:
  - Reorder tests
  - Mimic chain tests (now using joint-level affine + per-interface materialization)
  - Forward and reverse single-transmission tests (now expressed as two `TransmissionInstance`s)
  - Multi-input/multi-output grouped transmission
- New tests:
  - `TransmissionSubgraph` unit tests against a synthetic `TransmissionAnalysis` (no FK):
    - completable plan with pure transmission graph
    - completable plan that requires affine projection through mimic joints (position rule)
    - completable plan that requires affine projection with the offset dropped (velocity rule)
    - completable plan that requires affine projection with reversed direction (effort rule, registered explicitly by the test)
    - interface with no registered projection rule does **not** propagate via affine — surfaces as unreachable even when other interfaces on the same joint are known
    - viability filtering: a transmission with unreachable inputs is **not** counted as a candidate, even if its outputs are needed
    - unreachable-outputs reporting when needed outputs cannot be derived
    - `add_input` unblocks a previously-incomplete plan
    - `add_input` resolves a previously-ambiguous interface (and records the override in `input_overrides()`)
    - **Input wins over derived producers**: user supplies (A.position) and (B.position) where B mimics A — both are produced by `Input` (no false ambiguity from the affine projection)
    - **Input wins over Transmission**: user supplies an interface that is also produced by some transmission — the transmission is not selected for that interface
    - **`input_overrides()` is empty for initial inputs** but populated for overrides via `add_input`
    - **`add_input` with a duplicate interface** (already in `initial_inputs` or previously added): `requested_inputs()` shows the duplicate, `producer_of()` still points at the original index, no `input_overrides()` entry
    - **Duplicate interface in `initial_inputs`**: same first-occurrence-wins behavior — the second position exists in `requested_inputs()` but `producer_of()` points at the first
    - **Side-effect output**: a transmission `T` produces both K and I; user supplies a, b, I and asks for K; T is selected for K; the JointMap correctly outputs the user's I value, not T's computed I
    - mixed-quantity transmission (position-in / effort-out)
    - ambiguity accumulation: multiple ambiguous interfaces in one pass are all reported
    - non-Input ambiguity (Transmission vs AffineProjection) is still reported
    - non-ambiguous interfaces in an otherwise-ambiguous plan still return their unique producer
  - `TransmissionAnalysis` affine group tests:
    - isolated joint → `affine_root_of(j) == j`, `affine_group_members(j)` is `[j]`, `affine_transmission_of(j)` is the identity entry `(source=j, m=1, o=0)`
    - chain of mimics A → B → C → all share the same root (A); `affine_transmission_of(B)` and `affine_transmission_of(C)` give the **already-composed** flat (m, o) directly from A; no walking required
    - chain composition correctness across orderings: adding edges A→B→C in order vs B→C then A→B produces the same final per-joint flat relations for B and C
    - merging two non-trivial groups via a non-root edge: every member of the loser's group has its stored relation correctly recomposed in terms of the new (winner) root
    - `multiplier == 0` is rejected by `add_affine_transmission`
    - `source == target` (self-loop) is rejected
    - redundant consistent edge is silently no-op (winner == loser branch, debug-asserted to be within tolerance)
    - redundant inconsistent edge trips the debug assert (in debug builds only)
  - `compute_missing_input_resolutions` unit tests:
    - subgraph with no unreachable outputs → empty resolutions
    - one unreachable output with one producing transmission → one transmission alternative
    - one unreachable output with multiple producing transmissions → multiple transmission alternatives
    - unreachable output in a non-trivial affine group with a registered rule → `affine_root` populated
    - unreachable output in a trivial affine group → `affine_root == nullopt`
    - unreachable output whose interface id has no projection rule → no affine resolution offered
    - already-supplied alternatives are filtered out
  - `DefaultJointMapBuilder` end-to-end:
    - returns rich `JointMapBuildError` with `MissingInputs` when inputs are missing, including resolution hints
    - returns rich `JointMapBuildError` with `Ambiguous` when multiple viable producers exist
    - successfully emits an `AffineJointMap` for a pure-affine request
    - successfully emits a `CompositeJointMap` for a mixed affine + transmission request
    - **affine batching**: when the request has many affine outputs (e.g. 10 mimic joints + 1 transmission), the resulting `CompositeJointMap` contains a small number of segments — one consolidated `AffineJointMap` per "between transmissions" stretch — not one segment per output. Verifiable by walking the emitted segments and asserting each affine segment's `output_count() > 1` when there are multiple affine outputs in the same stage.
    - **affine + input pass-through fold**: when some outputs are direct input pass-throughs and others are affine projections, both kinds end up in the same `AffineJointMap` segment (input pass-throughs as identity rows `m=1, o=0`).

Existing tests are split out of `test_eigen_fk_mapper.cpp` into focused files as part of this work.

---

## Open items / out of scope

- `NamedStateInterfaceDefinition` convenience overloads on `JointMapBuilder` (deferred — canonical `StateInterfaceId` API only for now).
- Default-value-supplying joint map builder strategies (the seam exists; no concrete implementation).
- ros2_control controller helpers for sourcing missing state interfaces (acknowledged use case; future work).
- Ambiguity *resolution* policies (e.g. "prefer transmission X over Y when both are valid"). The current design always reports ambiguity rather than picking a winner — the user must remove the ambiguity from their setup. Smarter resolution can be added later as a builder-level concern, not a subgraph-level one.

