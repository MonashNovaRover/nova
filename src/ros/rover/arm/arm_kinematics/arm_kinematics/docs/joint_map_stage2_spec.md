# JointMap Stage 2 Implementation Spec

## Purpose

This document specifies Stage 2 of the `JointMap` refactor.

Stage 1 established the runtime abstraction boundary successfully:

- `JointMap` is now a runtime wrapper
- `AffineJointMap` is the fast reorder/mimic path
- `JointMapBuilder` is an interface
- `DefaultJointMapBuilder` is the shared default implementation

Stage 2 should introduce transmission-aware planning, but it should do so in a way that matches the rest of this package.

The intent of this stage is:

- keep the runtime API simple for users
- keep transmission topology purely joint-to-joint
- move quantity-specific complexity to build time only
- build transmission analysis once for the whole robot
- compile request-specific `JointMap` instances from cached indexed analysis
- align the transmission side of the library with the same kind of analysis-first approach already used by FK

The resulting user-facing shape should be:

- callers request a `JointMap` for a specific `JointQuantity`
- the returned `JointMap` still exposes one runtime `map(...)`
- failures happen while building the map, not during routine runtime use
- the final runtime compute structures do not carry `JointQuantity` as part of their execution API

## Existing References

This stage should follow patterns that already exist in the repository rather than inventing a separate architecture.

The most important references are:

- [implementation_guide.md](./implementation_guide.md)
- [joint_map_transmissions_plan.md](./joint_map_transmissions_plan.md)
- [analysis_tree.hpp](../include/arm_kinematics/forward/utilities/analysis_tree.hpp)
- [order.hpp](../include/arm_kinematics/utilities/order.hpp)

Those files point toward the architecture we should use:

- convert source metadata into a robot-wide analysis structure once
- use `Order<>` at analysis boundaries where named or reordered data is converted into indexed arrays
- store analysis data contiguously where possible
- derive smaller request-specific plans from the cached whole-robot analysis
- keep names out of the runtime hot path

## Context From The Original Overview

The broader architecture described in the earlier documentation still applies here:

- FK, collision, and IK are cooperating views over one shared robot description
- expensive structural work should happen during setup
- runtime execution should be array-oriented and preallocated
- `JointMap` bridges caller-facing joint spaces and compute-facing joint spaces

Stage 2 should preserve these properties:

1. setup-time analysis, runtime execution
2. one canonical name-to-id mapping for the robot-wide transmission problem
3. request-specific planning derived from cached analysis instead of reparsing raw metadata
4. no hidden string lookup in the hot path
5. compatibility with FK plugin customization seams

The design must also preserve the direction established in the transmission plan:

- do not overfit to ros2_control
- treat ros2_control as one metadata source, not the center of the architecture
- let FK plugins provide custom transmission definitions or custom builder behavior
- preserve the affine fast path
- preserve the ability to collapse many affine transmission relationships into one affine runtime map

## Scope

Stage 2 covers:

- introducing a lazily-built shared default `TransmissionAnalysis` cache
- defining indexed joint-to-joint transmission analysis data structures
- defining affine transmission analysis data for mimic relationships
- defining quantity-aware build-time selection for position and velocity
- defining directionality and reversibility rules
- defining grouped transmission execution semantics
- compiling request-specific indexed plans into runtime `JointMap` instances

Stage 2 does not need to solve:

- global optimization across arbitrary multi-stage pipelines
- aggressive scratch-buffer minimization
- all possible ros2_control transmission forms

## Non-Goals

Stage 2 must not:

- degrade the current affine fast path for reorder/mimic-only requests
- require ros2_control headers or types inside the core runtime transmission map
- keep names alive in runtime mapping structures where ids would suffice
- silently invent a mapping for ambiguous requests
- force affine transmission or mimic handling through `TransmissionModel`

Ensure:
- Where a name Order<std::string, id> is present, always immediately convert given named representations to indexed 
  representations immediately. Do not store strings anywhere but the name Order<std::string, id>.

## Design Goals

The Stage 2 design should satisfy these requirements:

1. `RobotModel` can build and cache a reusable default transmission analysis for the whole robot.
2. `ForwardKinematicsPlugin` can explicitly choose which `TransmissionAnalysis` it exposes to its builders.
3. Builders can derive request-specific plans from that chosen analysis without rebuilding it.
4. Builders can reject impossible or ambiguous mappings cleanly.
5. `TransmissionAnalysis` is quantity-agnostic and describes only structural joint relationships.
6. Position and velocity mapping are explicit build-time choices through `JointQuantity`.
7. Runtime transmission maps execute on grouped indexed data without carrying semantic quantity tags at execution time.
8. FK plugins can extend or specialize builder and analysis policy without changing the `JointMap` runtime API again.
   This is intended to be done through composition. JointMaps and builders should be unaware of FK plugins.

## Core Idea

The central shift in Stage 2 is not just:

`JointMap` planning should move from "per-output source lookup" to "space-to-space propagation planning."

It is also:

that planning should be based on a reusable robot-wide indexed analysis structure, not ad hoc string-heavy searches per request.

This should mirror the intent behind `AnalysisTree`:

- simplify source data into a representation suited to planning
- keep the full-robot representation cached
- derive smaller request-specific structures from that cached analysis
- compile the chosen structure into a runtime-friendly compute form

For transmissions, the equivalent should be `TransmissionAnalysis`.

In the revised design, `TransmissionAnalysis` should capture only structural joint relationships.
Quantity should not change the cached analysis topology, and is not involved in `TransmissionAnalysis`'s responsibilities.
It should only affect whether a builder can compile a requested `JointMap` from that topology.

This includes mimic joints.
Mimic joints should be treated as affine transmissions during setup-time analysis, not as builder-local rewrite rules.
However, they should not be represented through `TransmissionModel`.
They should use dedicated affine analysis structures so the planner can still compile the affine fast path directly.

Stage 2 should continue to think about affine transmission execution in grouped terms:

- `TransmissionAnalysis` may store many affine transmission relationships
- a request-specific affine planner should collapse all affine-only work for that request into one `AffinePlan` where possible
- `AffineJointMap` should execute that whole affine group together

Do not design toward one runtime affine stage per relationship.
The long-term direction is one `AffineJointMap` per affine execution segment.

## Proposed Stage 2 Architecture

## 1. Quantity Type

Quantity type should be explicit at build time.

Suggested shape:

```cpp
enum class JointQuantity {
  Position,
  Velocity,
};
```

This is important because:

- a valid position map may not imply a valid velocity map for the same joint topology
- velocity propagation rules may differ from position rules even when the joint connectivity is identical
- the caller should select the quantity it needs while building the map, then use one runtime `map(...)`
- once planning is complete, runtime compute structures should not need to know why that specific map was selected

## 2. Lightweight Transmission Definitions

Introduce a lightweight transmission abstraction owned by `arm_kinematics`, not by ros2_control. ros2_control should shape the design in any way.

However, this can be dealt with function overloads, and does not need to introduce unnecessary complexity through another class.
```
  void add_transmission(
    TransmissionModelId model_id,
    std::vector<JointId> && inputs, 
    std::vector<JointId> && outputs);
    
  // Convenience overload, which calls the above overload that uses JointId
  void add_transmission(
    TransmissionModelId model_id,
    span<cosnt std::string> && inputs, 
    span<const std::string> && outputs);
```

You will see the above snipped later on in this document.

- `std::string` joint identification is appropriate for imported source metadata such as URDF or plugin-provided named descriptions. This should be immediately converted to the indexed equivalent.
- `size_t` (with the alias `JointID` for clarity) is appropriate when a consumer already works in canonical `JointId` values, and is always preferred.

This template should remain a boundary-layer convenience, not a pattern that spreads through the full runtime and analysis stack.

## 3. Lightweight Transmission Builder Interface

Each transmission still needs a lightweight planning-layer interface, but Stage 2 should avoid forcing quantity semantics through the final runtime compute API.

Suggested conceptual shape:

```cpp
using JointId = size_t;
enum class PropagationDirection {
  Forward,
  Reverse,
};

class TransmissionModel {
public:
  virtual ~TransmissionModel() = default;

  virtual bool can_build(
    JointQuantity quantity,
    PropagationDirection direction) const noexcept = 0;

  virtual std::unique_ptr<const ComputeTransmission> build(
    JointQuantity quantity,
    PropagationDirection direction,
    span<const JointId> input_joint_ids,
    span<const JointId> output_joint_ids) const = 0;
};
```

Important properties:

- grouped, not scalar
- topology remains joint-based
- quantity matters only while checking support and building the compute object
- direction matters only while checking support and building the compute object
- runtime execution should use the built compute object, not repeatedly branch on quantity
- implementations may share logic between position and velocity where appropriate
- build products should be returned as `std::unique_ptr` and treated as move-only by default

This interface is for non-affine transmission compute only.
Affine transmission relationships, including normalized mimic chains, should not be modeled through `TransmissionModel`.
They should be stored directly in `TransmissionAnalysis` so they can compile to `AffineJointMap` without introducing runtime transmission compute or reversibility policy.

## 4. `TransmissionAnalysis`

Stage 2 should introduce a reusable robot-wide analysis structure, analogous in intent to `AnalysisTree`.

Recommended ownership:

- `RobotModel` owns and caches only a lazily-built default `TransmissionAnalysis`
- `TransmissionAnalysis` owns the normalized transmission models, preferably in a contiguous `std::vector<std::unique_ptr<TransmissionModel>>`
- `TransmissionAnalysis` also owns normalized affine transmission relationships for mimic joints
- `ForwardKinematicsPlugin` owns the choice of which `TransmissionAnalysis` it exposes to its builders
- builders query that chosen analysis rather than rebuilding it
- plugin-specific builders may augment or replace planning policy, but should still be able to reuse the shared default analysis when appropriate

This ownership split should be explicit:

- `RobotModel` is the source of shared robot-derived default analysis, not the final source of FK policy
- `ForwardKinematicsPlugin` is the source of the transmission analysis actually used by that FK implementation

That mirrors the existing FK seam better than treating `RobotModel` as the permanent owner of transmission-analysis
policy, while still preserving the efficiency benefit of one lazy shared default analysis reused across multiple FK
plugin instances.

Suggested FK seam:

```cpp
class ForwardKinematicsPlugin : public KinematicsBase {
public:
  [[nodiscard]] virtual const TransmissionAnalysis & get_transmission_analysis() const noexcept;
  [[nodiscard]] virtual const JointMapBuilder & get_joint_map_builder() const noexcept;
};
```

The default implementation can return the lazy shared default analysis from `RobotModel`, but specialized FK plugins
should be equally free to return an analysis they built, augmented, or wrapped themselves.

Suggested conceptual shape:

```cpp
using JointId = size_t;
using GroupId = size_t;
using ModelId = size_t;

class TransmissionAnalysis {
public:
  struct TransmissionInstance {
    ModelId model_id = 0;
    std::vector<JointId> input_joint_ids;
    std::vector<JointId> output_joint_ids;
  
    std::string name;   //< only used for logging to give info about invalid configurations!
    // forward and backward support determined by the TransmissionModel
  };

  [[nodiscard]] const Order<std::string, JointId> & joint_ids() const noexcept;
  
  [[nodiscard]] const std::vector<std::unique_ptr<TransmissionModel>> & models() const noexcept;
  TransmissionModelId add_model(std::unique_ptr<TransmissionModel> model);
  
  [[nodiscard]] const std::vector<TransmissionInstance> & transmissions() const noexcept;
  void add_transmission(
    TransmissionModelId model_id,
    std::vector<JointId> && inputs, 
    std::vector<JointId> && outputs,
    std::string name = "unnamed");
  /// Convenience overload, which calls the above overload that uses JointID
  void add_transmission(
    TransmissionModelId model_id,
    span<const std::string> && inputs, 
    span<const std::string> && outputs,
    std::string name = "unnamed");
  
  /// Canonical boundary mapping from named joints in descriptions to stable internal JointIds.
  [[nodiscard]] const Order<std::string, JointId> & joint_order() const noexcept { return joint_order_; }
  /// provides the JointID from joint_order_, adding it to the end of the order if it is not already present.
  JointId ensure_joint_id(const std::string & name);
  
private:
  // ...
};
```

The important point is not the exact final class shape.
The important point is that the robot-wide cache should be:

- indexed
- contiguous where possible
- derived once from definitions/models
- reusable for many `JointMap` build requests
- the long-lived ownership root for transmission models used during build

At this layer, the data should already be concretely `JointId`-based.

## 5. Canonical Name/Id Mapping

`TransmissionAnalysis` should use one canonical `Order<std::string, JointId>` only at the analysis boundary where named metadata is converted into internal ids.

That boundary mapping should be used for:

- assigning canonical joint ids across the full robot analysis
- converting requested input/output names into internal ids
- reconstructing readable names when diagnostics need them

After that conversion step, the analysis itself should remain in `JointId`-based contiguous arrays.
Group membership, plan stages, and compiled runtime structures should not keep `Order<>` members just to restate which joints they contain.

When a reverse mapping is needed for diagnostics, use `Order<std::string, JointId>::inverse` to obtain the corresponding `Order<JointId, std::string>` view rather than storing separate ad hoc lookup structures.

This is the intended use of `Order<>` in this design:

- use it once to cross the named boundary
- use it again when an actual ordering or permutation between arrays must be modeled
- do not use it as the default storage for internal analysis relationships

## 6. Indexed Analysis Structures

Once source metadata is imported, planning structures should become indexed rather than string-heavy.

Instead of this kind of structure:

```cpp
struct TransmissionEdge {
  size_t model_index;
  std::vector<std::string> consumed_names;
  std::vector<std::string> produced_names;
};
```

Stage 2 should prefer indexed structures more like:

```cpp
using JointId = size_t;
using GroupId = size_t;

struct TransmissionAnalysisEdge {
  GroupId group_id = 0;
  PropagationDirection direction;
  std::vector<JointId> consumed_joint_ids;
  std::vector<JointId> produced_joint_ids;
};
```

The exact data layout can still evolve, but the design direction should be:

- ids, not names
- contiguous vectors, not scattered maps
- analysis records separated from request-specific plans

## 7. Request-Specific Planning

Builders should not plan directly from raw transmission definitions.
They should plan from `TransmissionAnalysis`.

For a requested `(input_names, output_names, quantity)` build, the builder should:

1. convert requested names into canonical ids using the cached analysis boundary order
2. identify which outputs are satisfiable by affine mapping alone
3. identify which outputs require grouped transmission propagation
4. search the indexed analysis for a valid structural propagation plan
5. for each referenced transmission model, check whether it can build the requested quantity and propagation direction
6. reject the request if:
   - a required output cannot be reached
   - the needed direction is unsupported
   - the requested quantity is unsupported
   - multiple incompatible plans exist
7. compile the selected indexed plan into a runtime `JointMap`

This search does not need to be globally optimal yet.
It does need to be correct, deterministic, and derived from cached analysis.

For mixed affine and non-affine requests, the long-term direction is:

- identify maximal affine-only portions of the requested propagation
- compile each affine portion into one `AffineJointMap`
- separate those affine portions by grouped non-affine runtime stages only where genuinely required

Stage 2 does not need to finish that full automatic partitioning if doing so would force a design that later needs to
be undone.

If a request needs a specific ordering relationship between caller buffers and internal indexed arrays, that is an appropriate place to introduce request-local `Order<>` objects.
Those orders should describe buffer layout relationships, not replace the underlying `JointId`-based analysis structures.

## 8. Request Plan Representation

The request-specific planning result should also be indexed.
It should remain structural.
The requested `JointQuantity` should be carried by the build operation that compiles the plan, not by the plan data itself.

Suggested conceptual shape:

```cpp
using JointId = size_t;
using GroupId = size_t;

struct TransmissionPlanStage {
  GroupId group_id = 0;
  PropagationDirection direction;
  std::vector<JointId> consumed_joint_ids;
  std::vector<JointId> produced_joint_ids;
};

struct TransmissionPlan {
  std::vector<JointId> input_joint_ids;
  std::vector<JointId> output_joint_ids;
  std::vector<TransmissionPlanStage> stages;
};
```

This is the transmission-side equivalent of deriving a smaller request-specific structure from a full robot analysis tree.

## 9. Runtime Types

### `TransmissionJointMap`

Stage 2 should introduce a concrete runtime mapping type for grouped transmission propagation.

Suggested conceptual shape:

```cpp
class TransmissionJointMap {
public:
  explicit TransmissionJointMap(CompiledTransmissionPlan plan);

  void map(span<const double> inputs, span<float> outputs) const;

  [[nodiscard]] size_t input_count() const noexcept;
  [[nodiscard]] size_t output_count() const noexcept;
};
```

Important properties:

- runtime execution uses only indexed layouts
- no runtime string lookup
- no runtime quantity dispatch
- grouped stage execution is preserved
- preallocated scratch storage can be used where needed
- ownership should remain single-owner by default; use `std::unique_ptr` for built compute stages unless a concrete need for sharing appears later

Some transmissions may also need precomputed constants or captured state to execute efficiently.
That state should be owned by each `ComputeTransmission` instance at construction time, not allocated in the hot loop.

### `CompiledTransmissionPlan`

The runtime map should execute from a compiled indexed form.

Suggested shape:

```cpp
using InputIndex = size_t;
using OutputIndex = size_t;

class ComputeTransmission {
public:
  virtual ~ComputeTransmission() = default;
  virtual void compute(
    span<const float> inputs,
    span<float> outputs,
    span<float> scratch) const = 0;
};

struct CompiledTransmissionStage {
  std::unique_ptr<const ComputeTransmission> compute;
  std::vector<InputIndex> input_indices;
  size_t scratch_offset = 0;
  size_t scratch_size = 0;
  std::vector<OutputIndex> output_indices;
};

struct CompiledTransmissionPlan {
  InputIndex input_count = 0;
  OutputIndex output_count = 0;
  size_t scratch_size = 0;
  std::vector<CompiledTransmissionStage> stages;
};
```

Names should already be gone by this point.
Scratch should be allocated once when the runtime map is built or constructed, then reused for every compute call.

### `CompositeJointMap`

`CompositeJointMap` remains optional for Stage 2.
Introduce it only if it materially simplifies composition between:

- affine reorder/mimic stages
- grouped transmission stages
- final output reordering

## Builder Responsibilities In Stage 2

## 1. Result Types

Stage 2 should introduce explicit result types for analysis-derived planning.

Suggested shapes:

```cpp
using BuildJointMapResult = tl::expected<JointMap, std::string>;
using BuildTransmissionPlanResult = tl::expected<TransmissionPlan, std::string>;
```

These are justified because:

- ambiguous mappings must not silently succeed
- unsupported quantity/direction requests must fail clearly
- conflicts in transmission metadata should be surfaced at build time

## 2. `DefaultJointMapBuilder`

`DefaultJointMapBuilder` should gain:

- access to the cached `TransmissionAnalysis`
- indexed planning helpers
- a quantity-aware expected-returning build path

Suggested conceptual shape:

```cpp
class DefaultJointMapBuilder : public JointMapBuilder {
public:
  BuildJointMapResult build_expected(
    const std::vector<std::string> & input_names,
    const std::vector<std::string> & output_names,
    JointQuantity quantity) const;
};
```

Its existing `build()` can remain temporarily as a compatibility wrapper if needed, but the real Stage 2 planning path should be quantity-aware and analysis-driven.

## 3. Plugin-Specific Builder Composition

The plugin seam remains important.

FK plugins should be able to:

- reuse the default affine and transmission analysis path
- inject custom transmission models or definitions
- supply custom planning policy where needed

The important boundary is still:

- `RobotModel` owns shared robot-derived facts and a lazy default cached analysis
- FK plugins own policy for how that analysis is used to build maps, including which transmission analysis they expose

## ros2_control Integration Rules

Stage 2 should define these rules clearly:

1. ros2_control types must not appear inside `TransmissionJointMap`.
2. ros2_control types must not define `TransmissionAnalysis`.
3. ros2_control support should be implemented as adapters that produce lightweight transmission definitions/models consumed by the core analysis layer.
4. Custom FK plugins must be able to provide equivalent definitions/models without depending on ros2_control.
5. Either source may reasonably provide named definitions or already-indexed definitions, but the core analysis layer should normalize them to `JointId`-based storage.

That keeps the core architecture reusable.

## Error Semantics

Recommended failure categories:

- `unsupported_direction`
- `unsupported_quantity_type`
- `missing_required_input`
- `unreachable_output`
- `ambiguous_plan`
- `conflicting_definitions`

These failures should occur while building the `JointMap`, not during routine runtime mapping.

Warnings may still be appropriate for cases like:

- unused transmission definitions
- duplicate but equivalent definitions

But incorrect propagation must not degrade into best-effort behavior.

## Testing Requirements

Minimum Stage 2 coverage should include:

1. `TransmissionAnalysis` is built once and reused across multiple requests.
2. analysis uses stable canonical `JointId` assignments derived from one boundary `Order<std::string, JointId>`.
3. pure affine path is still chosen when no transmissions are required.
4. pure affine path remains correct for both position and velocity builds.
5. one transmission, forward direction.
6. one transmission, reverse direction.
7. unsupported reverse direction fails clearly.
8. unsupported velocity mapping fails clearly when quantity support differs.
9. one transmission where velocity propagation intentionally differs from position.
10. many-input/many-output grouped transmission executes as a unit.
11. ambiguous mapping fails clearly.
12. plugin-specific builders can reuse cached analysis and extend behavior.
13. ros2_control adapter tests remain separate from core analysis/runtime tests.

Also add planning-only tests for:

- canonical id assignment
- request name to id conversion
- request-local order construction when caller buffer ordering differs from canonical joint ordering
- direction selection
- quantity selection
- output reachability
- deterministic error messages for invalid requests

## Acceptance Criteria

Stage 2 is complete when:

- a reusable robot-wide `TransmissionAnalysis` exists
- `TransmissionAnalysis` topology is joint-based and quantity-agnostic
- `RobotModel` can lazily cache a clearly-named default analysis for shared reuse
- `ForwardKinematicsPlugin` can explicitly expose the transmission analysis its builders should consume
- transmission-backed mappings are planned from indexed cached analysis rather than raw string metadata
- callers request position or velocity behavior when building the `JointMap`
- runtime `JointMap` execution remains a single `map(...)` operation
- invalid or ambiguous requests fail cleanly at build time
- the affine fast path remains intact and preferred for simple cases
- affine-only requests are compiled as grouped affine execution, not as one runtime stage per affine relationship
- FK plugins can extend transmission-backed mapping behavior through their builders

## Deferred Decisions

Still defer these beyond Stage 2 if needed:

- global optimization of arbitrary multi-stage propagation pipelines
- automatic partitioning of arbitrary mixed affine/non-affine propagation into minimal execution segments, if the
  simpler Stage 2 implementation remains architecturally clean
- aggressive scratch-buffer reuse optimization
- broader builder API cleanup if `build_expected()` fully replaces `build()`

## Recommended Execution Order

1. Introduce `JointQuantity`, lightweight transmission definitions, and `TransmissionModel`.
3. Introduce quantity-agnostic `TransmissionAnalysis` with one boundary `Order<std::string, JointId>` and contiguous `JointId`-based internal storage.
4. Add `RobotModel` support for building and caching the whole-robot default `TransmissionAnalysis`, explicitly as a shared default cache.
5. Add `ForwardKinematicsPlugin` support for exposing the `TransmissionAnalysis` it wants its builders to consume, defaulting to the lazy `RobotModel` cache.
6. Add indexed structural request-planning structures derived from `TransmissionAnalysis`, introducing request-local `Order<>` objects only where actual buffer ordering/permutation needs to be modeled.
7. Introduce quantity-aware expected-based builder APIs that compile from a structural plan to a quantity-specific runtime map.
8. Keep `JointMap` as a single runtime type with one `map(...)`.
9. Implement `TransmissionJointMap` from compiled indexed plans using evaluators selected during build.
10. Add `CompositeJointMap` only if it materially simplifies composition.
11. Add plugin-specific builder extension tests.
12. Add ros2_control adapters last, on top of the lightweight analysis layer.

That keeps Stage 2 aligned with the existing architecture of the repository and with the analysis-first approach already established by FK.
