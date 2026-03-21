# JointMap Refactor Implementation Spec

## Purpose

This document turns the transmission-support plan into a concrete implementation spec for the first refactor stage and the immediate follow-on seams it depends on.

The goal of this stage is not to implement transmissions yet.
The goal is to reshape the code so transmission-capable joint-space propagation can be added later without fighting the current affine-only `JointMap` design.

This spec is intentionally narrower than the full plan:

- it focuses on the first implementation stage
- it defines concrete class boundaries
- it defines migration steps from the current code
- it avoids locking the project into ros2_control-specific semantics too early

## Scope

This spec covers:

- replacing the current monolithic `JointMap` representation with a runtime abstraction
- preserving the current fast affine reorder/mimic implementation as a first-class concrete type
- splitting builder responsibilities so custom FK plugins can own default mapping policy
- removing KDL from the future-facing `JointMap` API

This spec does not attempt to fully define:

- transmission planning
- reverse propagation semantics
- grouped transmission execution
- ros2_control transmission integration details

Those belong in the next design stage.

## Non-Goals

This stage must not:

- change existing FK behavior
- regress reorder or mimic performance in the default path
- force all joint maps through a general graph executor
- introduce ros2_control-specific runtime policy into the core abstraction

## Current Constraints From The Repository

The current implementation has these relevant properties:

- `RobotModel::get_joint_map_builder()` lazily constructs the shared default builder.
- `ForwardKinematicsPlugin::get_joint_map_builder()` already provides the right override seam for plugin-specific defaults.
- `EigenForwardKinematicsPlugin::make_tree()` only requires that the builder can produce a runtime map for a given input/output name pair.
- `JointMapBuilder::with_transmissions()` already parses transmission XML, but `build()` ignores it.
- `JointMap` is currently a single concrete type exposing `sources`, `multipliers`, and `offsets`.

This means the refactor can preserve the current tree-building flow if the new `JointMap` object remains return-by-value and callable via a stable `map()` method.

## Design Goals

The new design should satisfy these requirements:

1. Keep the caller-facing runtime contract simple.
2. Preserve the current affine fast path as an optimized concrete implementation.
3. Allow future runtime implementations with different internal behavior.
4. Allow builders to select the cheapest valid implementation.
5. Allow FK plugins to override the default builder policy cleanly.
6. Avoid exposing implementation-specific internals through the public `JointMap` API.

## Proposed Runtime API

## Public `JointMap`

`JointMap` should become a small value-type wrapper around an internal polymorphic implementation.

Proposed public shape:

```cpp
class JointMap {
public:
  JointMap() = default;

  template<class Impl>
  JointMap(Impl impl);

  void map(const span<double> & inputs, span<float> & outputs) const;

  [[nodiscard]] size_t input_count() const noexcept;
  [[nodiscard]] size_t output_count() const noexcept;
  [[nodiscard]] bool valid() const noexcept;

private:
  struct Concept;
  std::shared_ptr<const Concept> impl_;
};
```

Notes:

- `JointMap` should remain cheap to copy, but always prefer to move. Assume consumers follow good practices and move where appropriate.
- `JointMap` should remain return-by-value from builders.
- `JointMap` should not expose affine-specific arrays.
- `valid()` should replace implicit assumptions that the object is always populated.

~~I recommend `shared_ptr<const Concept>` rather than `unique_ptr` so copies remain cheap and semantics stay straightforward.~~
- A: I think we should implement true copying of the JointMaps with unique_ptr first, then move to using shared_ptr<> in the future if we deem it necessary.

## Internal `JointMap::Concept`

Proposed internal abstract base:

```cpp
struct JointMap::Concept {
  virtual ~Concept() = default;
  virtual void map(const span<double> inputs, span<float> outputs) const = 0;
  virtual size_t input_count() const noexcept = 0;
  virtual size_t output_count() const noexcept = 0;

  // Additional std::vector overload helper for converting to span
};
```

This is intentionally minimal.
It should not include builder-facing or planning-facing operations.

## Concrete Runtime Types

## `AffineJointMap`

This should be the direct successor to the current implementation.

Proposed public/private shape:

```cpp
class AffineJointMap {
public:
  AffineJointMap() = default;
  AffineJointMap(
    const std::vector<std::string> & input_names,
    const std::vector<std::string> & output_names,
    const std::map<std::string, std::shared_ptr<urdf::JointMimic>> & mimic_joints = {});

  static AffineJointMap identity(size_t element_count);

  void map(const std::vector<double> & inputs, std::vector<float> & outputs) const;

  [[nodiscard]] size_t input_count() const noexcept;
  [[nodiscard]] size_t output_count() const noexcept;

private:
  std::vector<size_t> sources_;
  std::vector<float> multipliers_;
  std::vector<float> offsets_;
  size_t input_count_ = 0;
  size_t output_count_ = 0;
};
```

Notes:

- move the current arrays behind private members
- preserve the current vectorized gather-then-affine behavior
- preserve the current mimic logic
- remove the `KDL::JntArray` overload entirely

This should remain the default output of the standard builder when no richer mapping is needed.

## `CompositeJointMap`

This type does not need to be implemented in Stage 1, but the `JointMap` wrapper must leave room for it.

Future conceptual shape:

```cpp
class CompositeJointMap {
public:
  explicit CompositeJointMap(std::vector<JointMap> stages);
  void map(const std::vector<double> & inputs, std::vector<float> & outputs) const;
};
```

Important:

- do not implement this yet unless needed for migration
- but do avoid designing `JointMap` in a way that would make this awkward later

## Proposed Builder API

## Public `JointMapBuilder`

`JointMapBuilder` should become a public abstract interface.

Proposed shape:

```cpp
class JointMapBuilder {
public:
  virtual ~JointMapBuilder() = default;

  [[nodiscard]] virtual JointMap build(
    const std::vector<std::string> & input_names,
    const std::vector<std::string> & output_names) const = 0;
};
```

This is the stable extension seam for:

- the default shared builder
- plugin-specific builders
- future transmission-aware builders

## `DefaultJointMapBuilder`

The current builder implementation should move into a new concrete type.

Proposed shape:

```cpp
class DefaultJointMapBuilder : public JointMapBuilder {
public:
  DefaultJointMapBuilder() = default;

  [[nodiscard]] JointMap build(
    const std::vector<std::string> & input_names,
    const std::vector<std::string> & output_names) const override;

  DefaultJointMapBuilder & with_urdf(const urdf::Model & urdf_model);
  DefaultJointMapBuilder & with_transmissions(const std::string & urdf_string, rclcpp::Logger logger);
  DefaultJointMapBuilder & with_transmissions_dangerous(const std::string & urdf_string);

private:
  std::vector<hardware_interface::TransmissionInfo> transmissions_;
  std::map<std::string, std::shared_ptr<urdf::JointMimic>> mimic_joints_;
};
```

Behavior in Stage 1:

- `build()` should always produce an `AffineJointMap`
- `transmissions_` may remain parsed-but-unused for now
- the type name should make it clear that this is the default shared implementation, not the only possible implementation

## Plugin-Specific Builder Contract

FK plugins should be able to provide their own builder implementations by overriding:

[`ForwardKinematicsPlugin::get_joint_map_builder()`](../include/arm_kinematics/forward/forward_kinematics_plugin.hpp)

No public API change is required to that virtual function except updating its return type to the new interface.

Conceptually:

- `RobotModel` owns shared robot-derived builder data
- FK plugins own default mapping policy for their backend

That distinction should be reflected in comments and docs.

## Ownership and Lifetime

## `RobotModel`

~~`RobotModel` should continue to lazily allocate and own the shared default builder instance.~~
- `RobotModel` should not own the default builder instance in the long term. We might be able to lazily retrieve this in the future, 
  but lazy retrieval should be through something like the PluginLoader, which could actually resolve a decent 'default' FK plugin instance to use.
- We could consider the use of an 'extended' robot model, for cases where an FK plugin is defined, to allow for the same convenience as with the previous design. 
- This refactor can be done later, and we can continue with this current plan for the time being.

Change:

- replace `std::unique_ptr<JointMapBuilder>` with `std::unique_ptr<DefaultJointMapBuilder>` internally
- expose it as `const JointMapBuilder &`

Proposed header-level contract:

```cpp
[[nodiscard]] const JointMapBuilder & get_joint_map_builder() const;
```

This preserves existing callers while making the ownership more accurate.

## `ForwardKinematicsPlugin`

`ForwardKinematicsPlugin::get_joint_map_builder()` should continue to default to:

```cpp
return get_robot_model().get_joint_map_builder();
```

but its documentation should explicitly say:

- this is the canonical source of the FK plugin's default runtime joint mapping policy
- different FK plugins may return different builders even when backed by the same `RobotModel`

## Migration Plan

## Step 1: introduce `AffineJointMap`

Create a new concrete type by moving the existing implementation out of `JointMap`.

Files likely involved:

- `include/arm_kinematics/joint_map/affine_joint_map.hpp`
- `src/joint_map/affine_joint_map.cpp`

Migration rules:

- copy current constructor logic unchanged
- copy current `map()` implementation unchanged except for removing KDL support
- preserve current mimic recursion logic

## Step 2: replace public `JointMap` with wrapper

Refactor:

- `include/arm_kinematics/joint_map/joint_map.hpp`

to define the wrapper/type-erased interface instead of the affine implementation itself.

Implementation file:

- `src/joint_map/joint_map.cpp`

This file should only contain:

- wrapper forwarding
- template helper plumbing or concrete model type bindings

It should not reimplement affine logic.

## Step 3: split `JointMapBuilder`

Refactor current builder code into:

- `JointMapBuilder` interface
- `DefaultJointMapBuilder` concrete implementation

Likely file layout:

- `include/arm_kinematics/joint_map/joint_map_builder.hpp`
- `include/arm_kinematics/joint_map/default_joint_map_builder.hpp`
- `src/joint_map/default_joint_map_builder.cpp`

If you want to minimize churn, you can keep the concrete type in the existing header first and split files later.
But the interface/concrete distinction should still exist immediately.

## Step 4: update `RobotModel`

Change `RobotModel` internals to lazily construct `DefaultJointMapBuilder`.

Behavior must stay the same:

- load mimic joints from URDF
- parse transmission XML
- expose the builder by abstract reference

## Step 5: update FK plugin interfaces

Update:

- `ForwardKinematicsPlugin`
- any concrete FK plugins

to reference the abstract builder API.

Expected impact:

- `EigenForwardKinematicsPlugin::make_tree()` should still compile with little or no behavior change
- the built runtime map remains return-by-value and stored directly in `TreeImpl`

## Step 6: update tests

Existing tests that inspect `JointMap` internals directly will need to change.

Recommended testing split:

- `AffineJointMap` tests validate `sources_`/`multipliers_`/`offsets_` behavior if you expose testing helpers or friend access
- public `JointMap` tests validate only observable runtime behavior

Do not keep public production API surface just to preserve old test inspection patterns.

## Proposed File Changes

Likely new files:

- `include/arm_kinematics/joint_map/affine_joint_map.hpp`
- `src/joint_map/affine_joint_map.cpp`
- `include/arm_kinematics/joint_map/default_joint_map_builder.hpp`
- `src/joint_map/default_joint_map_builder.cpp`

Likely modified files:

- `include/arm_kinematics/joint_map/joint_map.hpp`
- `src/joint_map/joint_map.cpp`
- `include/arm_kinematics/joint_map/joint_map_builder.hpp`
- `include/arm_kinematics/common/robot_model.hpp`
- `src/common/robot_model.cpp`
- `include/arm_kinematics/forward/forward_kinematics_plugin.hpp`
- `src/forward/forward_kinematics_plugin.cpp`
- `include/arm_kinematics/plugins/forward/eigen_forward_kinematics_plugin.hpp`
- `src/plugins/forward/eigen_forward_kinematics_plugin.cpp`

Likely tests to update:

- `test/forward/eigen/test_eigen_fk_mapper.cpp`

## API Compatibility Notes

Do not worry about making breaking changes to the API.

This is a breaking internal API change, but external usage can remain very similar if these are preserved:

- builders still return `JointMap` by value
- `JointMap` still exposes `map(inputs, outputs)`
- FK trees still store a `JointMap` directly

The main intentional break is:

- callers should no longer rely on public affine implementation fields
- KDL output overloads should go away

## Error Handling

Stage 1 should keep error handling behavior simple.

Recommendations:

- invalid maps should be represented by `JointMap{}` with `valid() == false`, or by always returning a valid identity/zero map if the builder contract already assumes success
- keep current missing-joint fallback behavior unchanged for now if preserving behavior is important

But note:

the current affine implementation silently zeros unresolved outputs by setting `multiplier = 0` and `offset = 0`.

That behavior should be explicitly documented in code comments during the refactor, because future transmission-aware builders may need stricter failure semantics.

## Performance Expectations

Stage 1 performance target:

- no measurable regression for the affine path in FK tree updates

This means:

- `AffineJointMap::map()` should stay essentially identical to the current implementation
- `JointMap` wrapper overhead should be limited to one virtual dispatch or equivalent erased call

That overhead should be negligible relative to the current gather-and-transform loop.

## Testing Requirements

Minimum tests for Stage 1:

1. `AffineJointMap` preserves pure reorder behavior.
2. `AffineJointMap` preserves mimic-joint behavior.
3. `DefaultJointMapBuilder::build()` still produces behavior-equivalent maps to the old builder.
4. `EigenForwardKinematicsPlugin` still produces the same FK results as before.
5. `RobotModel::get_joint_map_builder()` still lazily initializes and returns a usable builder.

Recommended additional tests:

6. `JointMap` wrapper copies remain valid and share behavior correctly.
7. default-constructed `JointMap` reports invalid state consistently.

## Acceptance Criteria

This stage is complete when:

- current FK tests pass with no expected behavior changes
- current mimic-driven `JointMap` behavior is preserved
- public runtime usage of `JointMap` no longer exposes affine internals
- `JointMapBuilder` is an extensible abstraction rather than a single concrete policy owner
- the codebase has a clear seam where a future `TransmissionJointMap` can be introduced without redesigning the API again

## Deferred Decisions

These decisions should be deferred to the next stage:

- how transmission stages declare reversibility
- how many-to-many grouped propagation is represented
- whether `CompositeJointMap` owns scratch buffers or allocates them externally
- whether builders return invalid maps or richer error types for unsupported propagation requests
- whether `JointMapBuilder::build()` should eventually return `tl::expected<JointMap, Error>`
- whether the default joint map builder should be removed from `RobotModel`

Those are important, but they should not block the Stage 1 refactor.

## Recommendation

The safest concrete implementation path is:

1. extract the current affine logic into `AffineJointMap`
2. wrap it behind a value-type `JointMap`
3. turn `JointMapBuilder` into an abstract interface
4. move the current builder logic into `DefaultJointMapBuilder`
5. update `RobotModel` and FK plugins to depend on the abstractions

That gives the project a stable base for the next design stage without forcing transmission semantics into the wrong representation.
