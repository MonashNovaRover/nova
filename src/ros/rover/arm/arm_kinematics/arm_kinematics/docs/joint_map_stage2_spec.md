# JointMap Stage 2 Implementation Spec

## Purpose

This document specifies Stage 2 of the `JointMap` refactor.

Stage 1 split the abstraction successfully:

- `JointMap` is now a runtime wrapper
- `AffineJointMap` is the fast reorder/mimic path
- `JointMapBuilder` is an interface
- `DefaultJointMapBuilder` is the shared default implementation

Stage 2 is the first stage that should introduce transmission-aware planning and runtime execution.

The goal of this stage is not to support every possible transmission system immediately.
The goal is to define the library's lightweight transmission model and the planning machinery needed to build valid `JointMap` instances for transmission-backed mappings.
It should also future-proof the API by making quantity type explicit at build time, so callers request either a position map or a velocity map and then use one runtime `map(...)` operation on the resulting `JointMap`.

## Context From The Original Overview

The broader architecture described in the implementation guide still matters here:

- FK, collision, and IK are cooperating views over one shared robot description
- the runtime path should stay array-oriented and preallocated
- expensive structural work should happen during setup
- `JointMap` exists to bridge between caller-facing and compute-facing joint spaces
- the same named joint-space relationship may need different rules for position and velocity propagation

That means Stage 2 should preserve these package-level properties:

1. setup-time planning, runtime execution
2. explicit alignment between named spaces and indexed runtime buffers
3. no hidden name lookup in the hot path
4. composition with existing FK plugin customization seams

The Stage 2 design must also respect the updated direction from the joint-map transmission plan:

- do not overfit to ros2_control
- define a lightweight transmission abstraction first
- allow FK plugins to provide custom transmission definitions through specialized builders
- treat ros2_control as one metadata source, not as the architectural center

## Scope

This stage covers:

- defining a lightweight transmission abstraction for `JointMap` planning
- defining directionality and reversibility rules
- defining grouped transmission execution semantics
- defining how builders plan a mapping from requested inputs to requested outputs
- introducing a transmission-capable runtime `JointMap` implementation
- defining how position and velocity mapping are represented separately in the planning model and builder request

This stage does not yet need to solve:

- global optimization of composed mapping pipelines
- scratch-buffer minimization across arbitrarily many stages
- replacing `RobotModel` as the owner of the default builder
- all possible ros2_control transmission forms

## Non-Goals

This stage must not:

- degrade the current affine fast path for reorder/mimic-only requests
- require ros2_control headers or types inside the transmission runtime implementation
- assume that every transmission is reversible
- silently invent a mapping for ambiguous requests

## Design Goals

The Stage 2 design should satisfy these requirements:

1. Builders can answer whether a requested mapping is valid.
2. Builders can reject impossible or ambiguous mappings cleanly.
3. Transmission-backed runtime maps operate on grouped inputs and outputs, not per-output independent lookups.
4. ros2_control support can be layered on top of the lightweight transmission abstraction later.
5. FK plugins can supply custom transmission definitions or custom builders without changing the runtime `JointMap` API again.
6. Position and velocity mapping are explicit builder-time choices, and the design does not assume they always share identical propagation rules.

## Core Idea

The central shift in Stage 2 is:

`JointMap` planning should move from "per-output source lookup" to "space-to-space propagation planning."

For affine maps, each output is independent.
For transmissions, that assumption breaks down.

So the builder must answer a different question:

"Given the names I know, and the names I need, what set of transforms can validly propagate values from the input space to the output space?"

That is closer to what `AnalysisTree` did for FK:

- simplify the problem into a representation suited to planning
- then compile that representation into a runtime-friendly compute form

Stage 2 should apply the same philosophy to transmission-backed joint mapping.
That propagation question should be interpreted per quantity type.
The valid plan for position may not be identical to the valid plan for velocity, even when the input and output names are the same.
The quantity choice should therefore be part of the builder request, not a runtime branch inside every caller.

## Proposed Stage 2 Architecture

## 1. Lightweight Transmission Definitions

Introduce a lightweight transmission abstraction owned by `arm_kinematics`, not by ros2_control.

Suggested conceptual shape:

```cpp
enum class JointQuantity {
  Position,
  Velocity,
};

struct TransmissionDefinition {
  std::string name;
  std::vector<std::string> inputs;
  std::vector<std::string> outputs;
  bool supports_forward = false;
  bool supports_reverse = false;
  bool same_velocity_rule_as_position = true;
};
```

This is only the descriptive layer.
It says:

- what values a transmission can consume
- what values it can produce
- what directions it supports
- whether velocity propagation should be treated as identical to position propagation by default

It does not yet say how the runtime math is executed.

## 2. Lightweight Transmission Runtime Interface

Each transmission needs a runtime evaluation object separate from the static definition.

Suggested conceptual shape:

```cpp
class TransmissionModel {
public:
  virtual ~TransmissionModel() = default;

  [[nodiscard]] virtual const TransmissionDefinition & definition() const noexcept = 0;

  virtual void forward(
    JointQuantity quantity,
    span<const float> inputs,
    span<float> outputs) const = 0;
  virtual void reverse(
    JointQuantity quantity,
    span<const float> outputs,
    span<float> inputs) const = 0;
};
```

Important:

- this interface is grouped, not scalar
- forward and reverse are separate operations
- quantity type is explicit at the model boundary
- implementations are allowed to support only one direction
- implementations are allowed to share logic internally when velocity is identical to position

This keeps the abstraction centered on propagation, not XML.

## 3. Transmission Registry / Collection

Builders need a way to access all available transmission models.

Suggested conceptual shape:

```cpp
struct TransmissionSet {
  std::vector<std::shared_ptr<const TransmissionModel>> models;
};
```

This can later be populated from:

- ros2_control transmission XML adapters
- plugin-specific transmission definitions
- custom solver-specific transmission sources

## 4. Transmission Planning Representation

The builder should not plan directly against raw XML or raw model objects.
It should plan against a simplified intermediate representation.

Suggested internal planning representation:

```cpp
struct TransmissionEdge {
  size_t model_index;
  enum class Direction { Forward, Reverse } direction;
  std::vector<std::string> consumed_names;
  std::vector<std::string> produced_names;
};
```

And the planning result:

```cpp
struct TransmissionPlan {
  std::vector<TransmissionEdge> stages;
  std::vector<std::string> final_output_order;
};
```

This is the transmission analogue of what `AnalysisTree` did for FK:

- it is easier to reason about than the original source format
- it can be validated before runtime execution
- it can later be compiled into a fast runtime map

## Builder Responsibilities In Stage 2

## 1. Add a Result Type

Stage 1 kept `JointMapBuilder::build()` simple and always returned a `JointMap`.
Stage 2 should upgrade the transmission-capable path to return an error-aware result.

Recommended shape:

```cpp
using BuildJointMapResult = tl::expected<JointMap, std::string>;
```

And for planning-only helpers:

```cpp
using BuildTransmissionPlanResult = tl::expected<TransmissionPlan, std::string>;
```

Reason:

- ambiguous mappings must not silently succeed
- unsupported reverse requests must not silently produce junk
- conflicting definitions should be surfaced cleanly

This was explicitly called out in the transmission plan notes and should now become part of the actual interface.

## 2. Planning Algorithm

For a requested `input_names -> output_names` mapping, the builder should:

1. Identify which requested outputs are already satisfiable by affine mapping alone.
2. Identify which requested outputs require transmission-backed propagation.
3. Search available transmission definitions for valid propagation paths.
4. Reject the request if:
   - a required output cannot be produced
   - more than one incompatible plan exists and the ambiguity is not resolvable
   - the necessary direction is unsupported
5. Compile the valid plan into a runtime `JointMap`.

This search does not need to be globally optimal yet.
It just needs to be correct and deterministic.

The builder should also be allowed to reject requests where position mapping is valid but velocity mapping is unsupported, because the caller should ask for the quantity type it actually needs and receive a quantity-specific `JointMap`.

## 3. Affine Fast Path Preservation

The builder must still prefer the current affine implementation whenever possible.

Required behavior:

- if a request is satisfiable by pure reorder/mimic, return `AffineJointMap`
- do not route simple requests through transmission planning unnecessarily

This should remain true for both:

- `DefaultJointMapBuilder`
- plugin-specific builders

## Runtime Types To Introduce

## 1. `TransmissionJointMap`

Stage 2 should introduce a new concrete runtime mapping type:

```cpp
class TransmissionJointMap {
public:
  explicit TransmissionJointMap(
    JointQuantity quantity,
    CompiledTransmissionPlan plan);

  void map(span<const double> inputs, span<float> outputs) const;

  [[nodiscard]] size_t input_count() const noexcept;
  [[nodiscard]] size_t output_count() const noexcept;
};
```

Its job is to execute grouped transmission propagation.

Important properties:

- it should operate on precomputed index layouts, not name lookups
- it should support forward and reverse stage execution as compiled by the builder
- it should preserve the quantity type selected at build time, so runtime callers still execute one `map(...)`
- it should use preallocated intermediate storage planned at setup time

## 2. `CompiledTransmissionPlan`

The runtime map should not execute directly from string-based planning data.
It should execute from an indexed compiled form.

Suggested shape:

```cpp
struct CompiledTransmissionStage {
  std::shared_ptr<const TransmissionModel> model;
  Direction direction;
  std::vector<size_t> input_indices;
  std::vector<size_t> output_indices;
};

struct CompiledTransmissionPlan {
  size_t input_count = 0;
  size_t output_count = 0;
  std::vector<CompiledTransmissionStage> stages;
  std::vector<float> initial_affine_buffer_template;
};
```

This is where names should disappear.

## 3. `CompositeJointMap`

Stage 2 should decide whether `CompositeJointMap` becomes real now or stays deferred.

Recommendation:

Implement it in Stage 2 only if it materially simplifies the builder output.

Useful cases:

- affine reorder into a transmission-space layout
- transmission stage(s)
- final affine reorder into caller-requested output layout

If introduced, it should operate over `JointMap` stages and a fixed set of scratch buffers allocated at construction.

## Proposed Stage 2 Builder Types

## 1. Extend `DefaultJointMapBuilder`

`DefaultJointMapBuilder` should gain:

- a collection of lightweight transmission definitions/models
- planning helpers
- a transmission-aware build path

Suggested additions:

```cpp
class DefaultJointMapBuilder : public JointMapBuilder {
public:
  BuildJointMapResult build_expected(
    const std::vector<std::string> & input_names,
    const std::vector<std::string> & output_names,
    JointQuantity quantity) const;

  DefaultJointMapBuilder & with_transmission_model(
    std::shared_ptr<const TransmissionModel> model);
};
```

Its existing `build()` can remain temporarily if you want a compatibility wrapper, but Stage 2 should move the real planning path onto an expected-returning API.

## 2. Plugin-Specific Builder Composition

The plugin delegation pattern should become concrete here.

Suggested shape:

```cpp
class SpecializedJointMapBuilder : public JointMapBuilder {
public:
  explicit SpecializedJointMapBuilder(const JointMapBuilder & base);

  BuildJointMapResult build_expected(...) const;
};
```

The builder should be able to:

- reuse the default affine/mimic behavior
- inject custom transmission models
- choose a custom plan when the backend requires it

This preserves the design goal that FK plugins own policy, while the default builder still owns shared robot-derived facts.

## ros2_control Integration Rules

Stage 2 should define these rules clearly:

1. ros2_control types must not appear inside `TransmissionJointMap`.
2. ros2_control types must not define the core transmission abstraction.
3. ros2_control support should be implemented as adapters that construct lightweight `TransmissionModel` instances.
4. Custom FK plugins must be able to provide the same kind of lightweight transmission models without depending on ros2_control.

That keeps the core architecture reusable.

## Error Semantics

Stage 2 must define explicit failure modes.

Recommended categories:

- `unsupported_direction`
- `unsupported_quantity_type`
- `missing_required_input`
- `unreachable_output`
- `ambiguous_plan`
- `conflicting_definitions`

The builder should fail rather than silently choose a dubious mapping.
The intended user-facing simplification is that this failure happens when building the `JointMap`, not during routine runtime mapping calls.

Warnings may still be appropriate in cases like:

- unused transmission definitions
- duplicate definitions that are identical

But incorrect propagation should not degrade into "best effort" behavior.

## Testing Requirements

Minimum Stage 2 test coverage:

1. pure affine path is still chosen when no transmissions are required
2. pure affine path remains correct for both position and velocity mapping
3. one transmission, forward direction
4. one transmission, reverse direction
5. unsupported reverse direction fails clearly
6. unsupported velocity mapping fails clearly when a transmission only supports position
7. one transmission where velocity propagation intentionally differs from position
8. many-input/many-output grouped transmission executes as a unit
9. ambiguous mapping fails clearly
10. plugin-specific builder can supply a custom transmission model
11. ros2_control adapter tests are separate from core transmission runtime tests

Also add planning-only tests for:

- graph construction
- direction selection
- quantity-type selection
- output reachability
- deterministic error messages for invalid requests

## Acceptance Criteria

Stage 2 is complete when:

- a lightweight transmission abstraction exists independent of ros2_control
- transmission-backed mappings can be planned and compiled into runtime `JointMap` instances
- callers request position or velocity behavior when building the `JointMap`, not by selecting between separate runtime mapping methods
- invalid or ambiguous requests fail cleanly
- the affine fast path remains intact and preferred for simple cases
- FK plugins can add custom transmission-backed mapping behavior through their builders

## Deferred Decisions

Still defer these beyond Stage 2 if needed:

- removal or relocation of the default builder from `RobotModel`
- global optimization of multi-stage mapping pipelines
- scratch-buffer minimization and aggressive runtime optimization
- broader builder API cleanup across all call sites if `build_expected()` replaces `build()` completely

## Recommended Execution Order

1. Define `JointQuantity`, `TransmissionDefinition`, and `TransmissionModel`.
2. Add a transmission planning representation and planning helpers.
3. Introduce expected-based planning/build APIs that take the requested quantity type.
4. Keep `JointMap` as a single runtime type with one `map(...)` operation.
5. Implement `TransmissionJointMap`.
6. Add `CompositeJointMap` only if the planning output needs it.
7. Add plugin-specific builder extension tests.
8. Add ros2_control adapters last, on top of the lightweight transmission layer.

That keeps Stage 2 aligned with the original architecture and avoids slipping back into a ros2_control-shaped design.
