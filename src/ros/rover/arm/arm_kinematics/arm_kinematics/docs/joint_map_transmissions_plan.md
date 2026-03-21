# JointMap Transmission Support Plan

## Intent

The intent of this change is to evolve `JointMap` from a single concrete affine reorder/mimic helper into the general joint-space conversion abstraction that the rest of the library already wants it to be.

Today, `JointMap` is effectively:

- a fast precomputed adapter from one ordered joint vector to another
- able to express reordering
- able to express mimic joints
- structurally prepared for transmission support, but not actually implementing it

The proposed change is to make `JointMap` able to represent more than one implementation strategy, so that:

- the fast SIMD-friendly reorder/mimic path stays cheap
- more complex many-to-many transmission logic becomes possible
- plugin-specific FK implementations can provide the most appropriate default `JointMapBuilder`
- ros2_control transmissions can be supported as one concrete mapping layer rather than being hard-coded into the current affine implementation

The design goal should be:

Preserve the current fast path for simple cases, while introducing an extensible framework for general known-joint-values -> related-joint-values propagation.

## Next Steps Summary

The immediate next steps should be:

1. Split the `JointMap` concept from the current concrete `sources`/`multipliers`/`offsets` implementation.
2. Introduce a composable `JointMap` interface or type-erased wrapper that can host multiple mapping strategies.
3. Make `JointMapBuilder` responsible for selecting or composing mapping implementations.
4. Move "default joint map builder" ownership from `RobotModel` toward FK plugins, with `RobotModel` remaining the fallback default source of shared URDF-derived mapping rules.
5. Add ros2_control transmission-backed mapping as a layered implementation after the abstraction boundary is in place.

## Difficulty Assessment

This is a medium-high difficulty refactor, not because the math for simple transmissions is especially hard, but because the current API shape assumes a single concrete mapping representation.

The hardest parts are not:

- parsing transmission XML, because that already exists in `JointMapBuilder`
- basic one-input one-output ratio/offset mappings

The hardest parts are:

- introducing many-to-many mapping semantics without destroying the current fast path
- defining how reversibility works when the caller asks for arbitrary input and output joint sets
- deciding where builder ownership should live so plugin-specific defaults are clean
- keeping runtime usage simple and real-time-friendly

My estimate:

- architecture/design difficulty: high
- implementation difficulty for the abstraction split: medium-high
- implementation difficulty for a first ros2_control transmission-backed concrete mapper: medium
- implementation difficulty for fully general reversible many-input/many-output transmission propagation: high

So the refactor is very feasible, but it should be done in stages rather than as a single rewrite.

## Current State In The Codebase

The current relevant pieces are:

- [`JointMap`](../include/arm_kinematics/joint_map/joint_map.hpp)
- [`JointMapBuilder`](../include/arm_kinematics/joint_map/joint_map_builder.hpp)
- [`RobotModel::get_joint_map_builder()`](../include/arm_kinematics/common/robot_model.hpp)
- [`ForwardKinematicsPlugin::get_joint_map_builder()`](../include/arm_kinematics/forward/forward_kinematics_plugin.hpp)
- [`EigenForwardKinematicsPlugin::make_tree()`](../src/plugins/forward/eigen_forward_kinematics_plugin.cpp)

The code currently behaves like this:

### 1. `RobotModel` owns the shared default builder

`RobotModel::get_joint_map_builder()` lazily creates one `JointMapBuilder`, populates it from:

- URDF mimic joints
- parsed ros2_control transmission XML

but only the mimic data is actually used by `JointMapBuilder::build()`.

### 2. FK plugins can already override the default builder

`ForwardKinematicsPlugin::get_joint_map_builder()` is virtual and currently defaults to:

- `get_robot_model().get_joint_map_builder()`

That seam is already exactly where plugin-specific mapping logic should hook in.

### 3. The concrete Eigen FK plugin uses the builder as an injected dependency

`EigenForwardKinematicsPlugin::make_tree()` accepts a `const JointMapBuilder &` and uses it to build the runtime joint map for its internal compute joint ordering.

That is good news, because the compute tree already does not care how the map is implemented.

### 4. `JointMap` is currently one concrete affine gather map

The existing `JointMap` stores:

- `sources`
- `multipliers`
- `offsets`

and applies:

`output[i] = input[sources[i]] * multipliers[i] + offsets[i]`

That is excellent for:

- reorder
- duplicate fan-out
- mimic

but it is structurally insufficient for:

- multiple inputs contributing to one output
- multiple outputs depending on one transmission solve
- reverse-direction propagation based on arbitrary requested input/output sets

### 5. `JointMapBuilder` already parses transmission definitions

`JointMapBuilder::with_transmissions()` populates `transmissions_`, but `build()` currently ignores that state and returns the simple concrete `JointMap`.

That means the parser work is already done, but the runtime abstraction is not.

## The Main Design Constraint

Transmission support changes the problem from:

"For each output, where does its value come from?"

to:

"Given some known values in one space, what sequence of transforms is needed to derive the requested values in another space?"

That is a different category of problem.

The current representation is output-local and affine.
Transmission propagation is graph-like and may require grouped evaluation.

That is why I do not think "just add more arrays to `JointMap`" is the right path.
The abstraction boundary itself needs to change.

## Recommended Architecture

## 1. Introduce an abstract runtime mapping interface

The first structural change should be separating:

- the `JointMap` concept
- the current fast affine implementation

I would recommend one of these shapes:

### Option A: abstract base class

Something like:

```cpp
class JointMap {
public:
  virtual ~JointMap() = default;
  virtual void map(const std::vector<double> & inputs, std::vector<float> & outputs) const = 0;
  virtual void map(const std::vector<double> & inputs, KDL::JntArray & outputs) const = 0;

  size_t input_count = 0;
  size_t output_count = 0;
};
```

Then concrete implementations such as:

- `AffineJointMap`
- `CompositeJointMap`
- `TransmissionJointMap`

### Option B: value wrapper around pimpl/type erasure

Something like:

```cpp
class JointMap {
public:
  template<class Impl>
  JointMap(Impl impl);

  void map(const std::vector<double> & inputs, std::vector<float> & outputs) const;
  void map(const std::vector<double> & inputs, KDL::JntArray & outputs) const;
};
```

backed by an internal polymorphic implementation.

I would lean toward Option B.
It preserves current caller ergonomics and avoids forcing ownership of `JointMap` through pointers everywhere.

## 2. Keep the current implementation as the fast path

The existing `JointMap` code should become a concrete implementation with a name that reflects what it really is.

Suggested names:

- `AffineJointMap`
- `GatherAffineJointMap`
- `SimdJointMap`

Its role:

- remain the default for reorder-only and mimic-only cases
- preserve the current vectorizable behavior
- stay trivially cheap to execute in the hot path

This is important because the majority of FK tree updates probably still fall into this category.

## 3. Add a compositional mapping implementation

If you want more complex implementations to "make use of previous implementations", you need a composition layer.

Suggested shape:

- `CompositeJointMap`

Conceptually:

- it owns an ordered list of mapping stages
- each stage maps from one intermediate joint space to the next

This gives you a clean model for:

- reorder/mimic first
- transmission propagation second
- plugin-specific custom derivation third

The key design decision is whether every stage must allocate an intermediate buffer or whether setup can pre-plan a minimal scratch layout.

My recommendation:

- start with explicit intermediate buffers during setup and reuse them at runtime
- optimize away unnecessary intermediate copies later

Do not start by trying to make the perfect zero-copy stage graph.

## 4. Separate builder interfaces from runtime maps

The builder side likely needs the same split as the runtime side.

The current `JointMapBuilder` is doing too many conceptually different jobs:

- storing mimic metadata
- parsing transmission metadata
- deciding how to build a map

I would refactor toward:

- `JointMapBuilder` as the abstract builder interface
- `DefaultJointMapBuilder` as the robot-model-backed default implementation
- optional specialized builders per FK plugin
- internal helper types for mimic and transmission graph analysis

Conceptually:

```cpp
class JointMapBuilder {
public:
  virtual ~JointMapBuilder() = default;
  virtual JointMap build(
    const std::vector<std::string> & input_names,
    const std::vector<std::string> & output_names) const = 0;
};
```

Then:

- `DefaultJointMapBuilder` handles mimic + ros2_control transmission-backed mapping
- a specialized FK plugin can override `get_joint_map_builder()` to return a custom builder

## 5. Make FK plugins the first-class source of the default builder

You explicitly want the default builder to come from the FK plugin implementation.
That makes sense, and the current code is already close.

Today:

- `ForwardKinematicsPlugin::get_joint_map_builder()` defaults to `RobotModel`
- callers who use `make_tree(joint_names, base_link_name, frames)` effectively use the FK plugin's default builder

What should change is mostly the ownership story and the documentation:

- plugin-facing code should treat the FK plugin as the source of the default builder
- `RobotModel` should remain the source of shared robot-derived default mapping metadata

In other words:

- `RobotModel` owns shared raw facts
- the FK plugin chooses how those facts become an actual builder

That avoids hard-coding the mapping policy at the robot-model layer.

## Transmission Support Model

## 1. Model transmissions as graph edges between spaces

To support many-input and many-output transmissions, think in terms of a graph rather than direct per-output lookup.

Each transmission definition should be treated as a transform between two related spaces, for example:

- actuator space
- joint space

The builder's job becomes:

1. identify what names are known from the requested inputs
2. identify what names are needed for the requested outputs
3. plan a propagation path across available transforms
4. emit a runtime `JointMap` implementation capable of executing that path

This is the only clean way to handle arbitrary directionality.

## 2. Directionality must be explicit

Your note about acting in reverse depending on requested inputs and outputs is the core difficulty.

A transmission cannot simply be "applied".
It must be applied in a direction.

So each mapping stage should expose, conceptually:

- what named variables it can consume
- what named variables it can produce
- whether it supports forward propagation
- whether it supports reverse propagation

For ros2_control transmissions, the builder may need adapters that know how to:

- propagate actuator -> joint
- propagate joint -> actuator

depending on the transmission type and what was requested.

Not every stage should be assumed reversible.
Reversibility should be a capability, not a default assumption.

## 3. Some transmission groups must be solved as a unit

For many-to-many transmissions, per-output independent evaluation will be wrong.

Examples:

- differential-like systems
- coupled joints
- actuator groups that jointly define one joint set

That means a transmission-backed joint map stage likely needs:

- grouped inputs
- grouped outputs
- one stage function that computes an entire output block together

This is another reason the current `sources`/`multipliers`/`offsets` representation cannot be stretched far enough.

## Proposed Concrete Runtime Layers

I would aim for three concrete runtime layers, which aligns well with your idea.

## 1. `AffineJointMap`

Purpose:

- reorder
- subset selection
- duplication
- mimic

Characteristics:

- very fast
- SIMD-friendly
- stateless except for precomputed arrays

This is basically the current implementation.

## 2. `TransmissionJointMap`

Purpose:

- grouped propagation through one or more transmission definitions
- support multi-input and multi-output relationships
- support explicit forward and reverse evaluation where valid

Characteristics:

- likely uses small intermediate scratch buffers
- stage evaluation may be per-group rather than per-output
- probably cannot be vectorized the same way as the affine path

This should be the base abstraction for both ros2_control transmissions and custom plugin-defined transmission logic.

## 3. `CompositeJointMap`

Purpose:

- chain multiple map implementations together

Example uses:

- caller order -> plugin internal order through `AffineJointMap`
- then plugin internal order -> actuator/joint coupled space through `TransmissionJointMap`
- or the reverse sequence depending on requested IO spaces

This gives you composition without forcing one implementation to know everything.

## Proposed Builder Layers

## 1. `DefaultJointMapBuilder`

Responsibilities:

- gather mimic information from URDF
- gather transmission information from URDF ros2_control tags
- build the simplest valid map for a requested IO pair

Behavior:

- if the request is representable as affine reorder/mimic only, return `AffineJointMap`
- if transmission propagation is required, return `TransmissionJointMap` or `CompositeJointMap`

## 2. Plugin-specific builders

Responsibilities:

- introduce custom joint-space transforms required by a specialized FK backend
- reuse the default builder where possible

This matches your idea that specialized implementations can make use of previous implementations.

The cleanest way to do that is composition:

- plugin builder wraps or delegates to the default builder
- then adds its own stage if needed

## 3. Builder delegation model

I would explicitly support a model like:

```cpp
class SpecializedJointMapBuilder : public JointMapBuilder {
public:
  explicit SpecializedJointMapBuilder(const JointMapBuilder & base);
  JointMap build(...) const override;
};
```

That will make layering much easier than trying to subclass one monolithic builder with too much internal state.

## API Changes Likely Needed

## 1. `JointMap`

Will need to stop being a single concrete struct with exposed arrays.

That is a breaking API change, but I think it is justified.

If you want to keep access to the affine arrays for testing or optimization work, expose them only on the affine concrete type.

## 2. `JointMapBuilder`

Likely needs to become polymorphic, or at least return a polymorphic `JointMap`.

The existing return-by-value API is still fine if `JointMap` becomes a value wrapper around an internal implementation pointer.

## 3. `ForwardKinematicsPlugin`

`get_joint_map_builder()` is already virtual and is the right seam.
The main change here is conceptual:

- document that this is the canonical source of the plugin's default builder
- use it consistently in new code paths

## 4. `RobotModel`

`RobotModel::get_joint_map_builder()` probably should remain, but be reframed as:

- the shared default builder factory
- not necessarily the final policy owner for a specific FK backend

I would not remove it immediately.
It is still useful as the common backing source of URDF-derived mapping rules.

## Suggested Implementation Stages

## Stage 1: abstraction split

Deliverables:

- introduce polymorphic or type-erased `JointMap`
- move current implementation into `AffineJointMap`
- keep all existing behavior working

Risk:

- mostly mechanical refactor, but touches FK plugin code and tests

Success criterion:

- no behavioral change
- all current tests still pass

## Stage 2: builder split

Deliverables:

- make `JointMapBuilder` an interface or clear extension point
- add `DefaultJointMapBuilder`
- keep `RobotModel` as the owner of the default shared builder

Risk:

- low-medium

Success criterion:

- FK plugins still build trees exactly as before
- specialized plugins can now supply custom builders cleanly

## Stage 3: transmission planning model

Deliverables:

- represent transmission metadata in a form suitable for planning propagation
- define directionality and reversibility rules
- define grouped evaluation semantics

Risk:

- high design risk

Success criterion:

- you can answer, for any requested input/output name sets, whether a valid propagation plan exists

## Stage 4: ros2_control transmission-backed runtime map

Deliverables:

- implement `TransmissionJointMap` for the supported ros2_control transmission types
- allow forward and reverse mapping where valid

Risk:

- medium-high, depending on how many transmission types you want initially

Success criterion:

- end-to-end tests show actuator/joint mappings working in both supported directions

## Stage 5: composition and optimization

Deliverables:

- `CompositeJointMap`
- builder choosing cheapest valid implementation
- optional scratch-buffer reuse improvements

Risk:

- medium

Success criterion:

- complex cases work without regressing the simple affine fast path

## Testing Strategy

This change needs much stronger test coverage than the current simple reorder/mimic implementation.

At minimum, add tests for:

- pure reorder
- mimic chain
- affine fast path still chosen when no transmissions are involved
- one transmission, forward direction
- one transmission, reverse direction
- many-input/many-output grouped transmission
- impossible mapping requests fail clearly
- composition of affine stage plus transmission stage
- plugin-specific builder overriding the default builder

I would also add builder-planning tests that operate only on names and metadata, independent of FK, so transmission planning bugs can be debugged without a full solver stack.

## Main Risks

The main risks I see are:

### 1. Overfitting the abstraction to ros2_control transmissions

If the abstraction is too close to current ros2_control parser structures, plugin-specific custom mappings will become awkward.

The abstraction should be about propagation stages, not about XML.

### 2. Breaking the fast path

If every `JointMap` becomes a graph executor, even simple reorder/mimic-only FK updates will get slower and more complicated.

The fast affine implementation should remain a first-class concrete type.

### 3. Ambiguous reverse mappings

Some transmission systems may not be fully reversible from arbitrary partial inputs.

The builder should be allowed to say:

- mapping is valid
- mapping is invalid
- mapping is ambiguous or unsupported

Do not force every request to succeed.

### 4. Smuggling too much policy into `RobotModel`

You explicitly want default builder policy to come from FK plugins.
That is the right instinct.
`RobotModel` should stay as shared data ownership, not become the global owner of backend-specific mapping policy.

## Recommendation

I think this is worth doing, and the structure you are proposing is directionally correct.

Specifically:

- yes, split the current implementation from the abstract `JointMap` concept
- yes, keep a SIMD-friendly concrete affine implementation
- yes, add a more general transmission-capable implementation
- yes, allow specialized FK plugins to provide a custom builder that can reuse a previous/default implementation
- yes, model transmission propagation as directional and grouped rather than per-output independent

What I would change in your current instinct is mostly this:

Do not start by implementing ros2_control transmission support directly inside the current `JointMap`.
Start by making the runtime and builder abstractions capable of hosting multiple implementations.

That is the prerequisite that will make the rest of the work tractable.

## Recommended Immediate Work Items

If I were sequencing the actual engineering work, I would do these first:

1. Refactor the current `JointMap` into `AffineJointMap` plus a wrapper/interface.
2. Refactor `JointMapBuilder` into an extensible abstraction without changing behavior.
3. Add tests that lock down current reorder+mimic behavior.
4. Add a transmission planning document or internal design note defining forward/reverse/group semantics.
5. Only then implement the first transmission-backed runtime map.

That ordering keeps the risk under control and prevents transmission logic from being forced into the wrong abstraction.
