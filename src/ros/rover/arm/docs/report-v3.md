# Integrating `arm_kinematics` into `arm_controllers` for `ros2_control`

## Executive Summary

The overall direction in `report-v2.md` is right:

- MoveIt setup is duplicated across `nova_twistmapper` and `nova_path_planner`
- the MoveIt joint-ordering hack should disappear on the FK side
- controller-owned `PluginLoader` / `RobotModel` is a reasonable initial ownership model
- reusable `ros2_control` integration helpers belong with `arm_kinematics`

However, `report-v2.md` overstates how ready `arm_kinematics` already is for migration.
The remaining work is not just adapter glue. There are still core integration blockers,
mostly around collision semantics and API shape.

The most important correction is this:

`arm_kinematics` already provides the right architectural basis for replacing most MoveIt
usage in these controllers, but it is not yet migration-ready for all three controllers
without additional work in collision semantics, planner-facing IK API shape, and
`ros2_control` interface helpers.

This report keeps the good parts of `report-v2.md`, corrects the weak claims, and proposes
a migration order that reduces risk.

## Scope and Non-Goals

This report assumes the following design choices are intentional:

- analytical IK is the only IK approach in scope
- numerical search is not a target design
- MoveIt-style `searchPositionIK(..., timeout, ..., error_codes)` is not the long-term API
- twistmapper should not compute more FK frames than necessary in the real-time loop

Under those constraints, the question is not "how do we emulate MoveIt exactly?"
The question is "what needs to exist in `arm_kinematics` and its controller adapters so the
controllers can migrate cleanly without inheriting MoveIt's API and setup model?"

## Current State of the Controllers

The current controller situation is substantially as described in `report-v2.md`.

### `nova_twistmapper`

`nova_twistmapper` currently depends on MoveIt for:

- URDF + SRDF model construction
- FK lookup for end effector and twist reference frame
- IK solution of target pose
- self-collision checking
- joint-order coupling between controller handles and MoveIt joint-group order

It also duplicates a large amount of controller-side setup and interface acquisition code.

### `nova_path_planner`

`nova_path_planner` duplicates almost the same MoveIt setup as `nova_twistmapper`, then
uses MoveIt FK and IK during path generation and MoveIt collision checks for validating
interpolated joint-space motion.

### `nova_arm_controller`

`nova_arm_controller` is simpler. It does not need IK, but it does use a MoveIt-backed
`SelfCollisionLimiter`. It also has more complex interface acquisition than the other two
controllers because it conditionally requires both position and velocity state interfaces
per joint.

That makes it the best first migration target, but it also means the proposed shared
interface utilities need to support richer per-joint bundles than `report-v2.md` currently
describes.

## What Is Already Ready

These parts of `report-v2.md` are basically correct.

### `RobotModel` and FK-side joint mapping

The FK side is the strongest part of the current design.

`RobotModel`, `TransmissionAnalysis`, `AnalysisTree`, `ForwardKinematicsPlugin`, and
`JointMap` already support the important architectural shift:

- controller-owned input order
- FK-time reordering through `JointMap`
- mimic / transmission-aware mapping
- frame-oriented FK trees built from a shared robot model

This is the real replacement for the MoveIt joint-group-order dependency and the
post-acquisition `std::sort(...)` hack in `nova_twistmapper` and `nova_path_planner`.

### Controller-owned `PluginLoader`

For initial integration, each controller owning its own `PluginLoader` and `RobotModel`
is the right choice.

Benefits:

- lifecycle ownership is simple
- configuration remains controller-local
- it avoids inventing a shared singleton too early
- there is no demonstrated performance need to centralize this yet

### `arm_kinematics::ros2_control` integration helpers belong here

Reusable controller helpers should live alongside `arm_kinematics`, not as a separate
half-owned utility namespace.

These helpers are not generic enough to justify a new independent utility package, and they
directly support the controller integration story for this library.

The right home is a small integration layer such as:

```cpp
namespace arm_kinematics::ros2_control {
// helpers
}
```

## What `report-v2.md` Missed or Overstated

## 1. Collision semantics are not migration-ready yet

This is the biggest omission.

`report-v2.md` treats collision as mostly a factory and convenience gap. That is too mild.
The default collision behavior in `arm_kinematics` does not yet match the current
controller behavior closely enough to call the migration path ready.

Today:

- `PluginLoader::make_collision(...)` builds collider definitions from the URDF
- the default `AllowedCollisionMatrix` comes from `ColliderDefinitions`
- that default ACM only auto-allows same-link collider pairs

Current controllers do something stronger:

- they derive additional allowed pairs from a default or zero pose
- those allowed pairs become part of the effective self-collision policy

That is a semantic mismatch, not just missing sugar.

### Design implication

Before the controllers migrate off MoveIt collision, `arm_kinematics` needs a collision
construction path that can reproduce controller-equivalent semantics.

That should include:

- optional probing from a configured zero/default pose
- explicit allowed-pair overrides
- stable parameterization
- stable diagnostics when the collision tree cannot be built from available inputs

### Concrete codebase note

There is also a current implementation bug that matters here:

- `FclCollisionPlugin::collide(colliding_pairs, ...)` does not currently collect pairs
  correctly due to calling the wrong callback

That is a code bug, not a design problem, but it should be called out as a blocker in the
report because the proposed ACM-generation path depends on pair collection.

## 2. The planner still speaks MoveIt

This is the second major omission.

The problem with `nova_path_planner` is not "it needs a numerical search equivalent."
The real problem is that its current control flow is still shaped around the MoveIt IK API.

Today the planner expects something like:

- pose in
- seed state in
- timeout in
- solution or failure
- MoveIt-flavored error code on failure

That API shape should not be preserved.

### Recommended direction

Before migrating `nova_path_planner`, refactor the planner-facing IK boundary to something
explicit and library-native, for example:

```cpp
enum class IKFailureKind {
  NoSolution,
  OutsideWorkspace,
  Singular,
  InvalidSeed,
  InvalidTarget,
};

struct IKFailure {
  IKFailureKind kind;
  std::string message;
};

using IKResult = tl::expected<std::vector<double>, IKFailure>;
```

The exact shape can vary, but the design goal should be:

- no MoveIt-style `searchPositionIK(...)`
- no MoveIt error-code object
- explicit success/failure result
- room to express analytical failure reasons later

### Practical consequence

`report-v2.md` should stop describing `InverseKinematicsPlugin` as a direct replacement for
the planner's current `searchPositionIK(...)` usage.

It is the right long-term abstraction, but the planner needs an API refactor before the
backend swap is honest.

## 3. Velocity IK should be treated as plugin-dependent, not peer-default behavior

The current default `get_velocity_ik()` implementation is valid enough as a fallback:

- apply twist to pose over `dt`
- solve position IK for the resulting pose
- finite-difference the joint positions

That is not inherently a design problem.

The real issue is how it is presented.

`report-v2.md` treats position IK mode and velocity IK mode as near-equal options for
twistmapper. That is too strong.

### Better wording

The report should say:

- initial twistmapper migration should use position IK mode
- velocity IK support can exist as an optional plugin-dependent mode
- if a plugin overrides `get_velocity_ik()` analytically, that is great
- otherwise the default fallback is acceptable, but it inherits the branch behavior and
  discontinuities of the position IK path

That is a real design caution, not a rejection of the feature.

## 4. The proposed `ros2_control` helpers are too weak for `nova_arm_controller`

The helper direction in `report-v2.md` is good, but the helper set is incomplete.

The report proposes:

- interface-name construction
- ordered ref acquisition by exact interface name

That is enough for the basic twistmapper/path-planner shape, but not for
`nova_arm_controller`.

`nova_arm_controller` needs, per joint:

- optional position state ref
- optional velocity state ref
- one command ref

and those requirements depend on parameters.

### Recommended change

The report should add a richer helper layer, for example:

```cpp
namespace arm_kinematics::ros2_control {

struct JointInterfaceBundle {
  std::string joint_name;
  const hardware_interface::LoanedStateInterface * position = nullptr;
  const hardware_interface::LoanedStateInterface * velocity = nullptr;
  hardware_interface::LoanedCommandInterface * command = nullptr;
};

tl::expected<std::vector<JointInterfaceBundle>, InterfaceLookupError>
find_joint_interface_bundles(...);

}
```

This does not replace the simpler helpers. It supplements them so the first migration target
does not have to keep bespoke acquisition forever.

## 5. The path-collision helper should be reimplemented, not extracted as-is

`report-v2.md` is correct that path collision checking belongs in shared code.

But it should explicitly say not to lift the current duplicated algorithm unchanged.

The current controller implementations compute the interpolation count incorrectly.
The shared helper should be defined in terms of:

- maximum joint displacement between start and end
- `iterations = ceil(max_displacement / step_size)`
- checking each intermediate state and the endpoint

So the right recommendation is:

- move this into shared code
- fix the stepping logic while doing so
- test it once, instead of preserving duplicated buggy controller logic

## 6. Diagnostics should prefer `expected` over throws on the integration path

This part of `report-v2.md` is correct in spirit but too optimistic in how it describes the
current state.

The integration path should prefer:

- `tl::expected`
- structured failure payloads
- explicit controller handling of setup-time failures

That applies especially to:

- FK tree construction
- collision-manager construction
- planner-facing IK calls

There is a concrete example already in the codebase.

`arm_kinematics/arm_kinematics/src/plugin_loader.cpp:38` contains a helper,
`unwrap_make_tree_result`, that converts a `tl::expected` failure from
`ForwardKinematicsPlugin::make_tree()` into a `std::runtime_error` throw. This helper is
called from `make_collision`, so any controller calling `make_collision_manager()` during
`on_configure` will get an unhandled exception rather than a structured failure if the FK
tree cannot be built.

The fix is to remove `unwrap_make_tree_result` and propagate the typed `MakeTreeError`
through `make_collision_manager()`'s own `tl::expected` return — no string flattening:

```cpp
auto tree_result = fk->make_tree(...);
if (!tree_result) {
  return tl::unexpected(
    MakeTreeError{MakeTreeError::JointMapBuildFailed{std::move(tree_result.error())}});
}
```

That makes `make_collision_manager()` consistent with the rest of the `arm_kinematics` API
and removes a hidden exception crossing from library code into controller `on_configure`.

### Error type redesign

This fix also motivates a broader redesign of `JointMapBuildError` and `MakeTreeError`.

The current shape — `enum class Kind` discriminator plus optional data fields on a shared
struct — is error-prone: the compiler does not prevent accessing `unproducible_outputs` when
`kind == Ambiguous`. The better shape is `std::variant` with one concrete type per failure
mode. Each concrete type lives as a nested struct, carries exactly its own data, and has a
`format() const` method. The outer struct holds the variant and provides a templated
converting constructor so callers never need to touch the `.value` member directly:

```cpp
struct JointMapBuildError {
  struct MissingInputs {
    std::vector<StateInterfaceDefinition> unproducible_outputs;
    std::vector<MissingInputResolution>   resolutions;
    std::string format() const;
  };
  struct Ambiguous {
    std::vector<producers::AmbiguousInterface> ambiguous_interfaces;
    std::string format() const;
  };
  struct UnknownJoint {
    std::vector<JointId> unknown_joints;
    std::string format() const;
  };

  using Variant = std::variant<MissingInputs, Ambiguous, UnknownJoint>;
  Variant value;

  template <typename T>
  JointMapBuildError(T && t) : value(std::forward<T>(t)) {}
};
```

`MakeTreeError` follows the same pattern:

```cpp
struct MakeTreeError {
  struct JointMapBuildFailed { JointMapBuildError error; std::string format() const; };
  struct FrameTreeFailed     { std::string detail;       std::string format() const; };
  struct UnknownJoint        { std::vector<JointId> unknown_joints; std::string format() const; };

  using Variant = std::variant<JointMapBuildFailed, FrameTreeFailed, UnknownJoint>;
  Variant value;

  template <typename T>
  MakeTreeError(T && t) : value(std::forward<T>(t)) {}
};
```

With this shape, constructing and returning errors is direct:

```cpp
return tl::unexpected(JointMapBuildError::MissingInputs{unproducible, resolutions});
```

And consuming them is explicit via `std::visit`, with no risk of accessing the wrong fields:

```cpp
std::visit([](const auto & e) { RCLCPP_ERROR(logger, "%s", e.format().c_str()); },
           err.value);
```

## Revised Utility Proposal

The right utility story is slightly broader than `report-v2.md`.

These should live in `arm_kinematics::ros2_control`.

### Utility A: interface-name construction

This is still worth doing.

Examples:

- state interface names for a set of joints and interface types
- command interface names with optional chained-controller prefix
- `NamedStateInterfaceDefinition` overloads for FK-oriented controllers

### Utility B: exact-name ref acquisition

This is still useful for:

- twistmapper
- path planner
- any controller with a simple ordered list of required interfaces

### Utility C: per-joint bundled interface acquisition

This should be added for controllers like `nova_arm_controller`.

It should support:

- required / optional state-interface lookup by type
- one command interface per joint
- structured missing-interface diagnostics

## Revised Per-Controller Migration Plan

### `nova_arm_controller`

This should be migrated first.

Changes:

- replace MoveIt-backed self-collision internals with `arm_kinematics` collision
- keep a thin limiter adapter initially if that preserves the current controller flow
- use richer bundled interface acquisition helpers
- do not drag IK concerns into this migration

Why first:

- smallest MoveIt surface
- best place to validate collision parity
- best place to validate new `ros2_control` helper layer

### `nova_twistmapper`

This should be migrated second.

Changes:

- replace MoveIt setup with `PluginLoader`
- replace FK calls with `ForwardKinematicsPlugin::Tree`
- replace position IK with `InverseKinematicsPlugin::get_position_ik()`
- replace MoveIt collision checks with `CollisionManager`
- keep the twist-frame tree swap design

Important qualification:

- initial migration should use position IK mode only
- velocity IK can be added as a plugin-dependent option later

### `nova_path_planner`

This should be migrated last.

Changes:

- first refactor planner-side IK consumption to a non-MoveIt result type
- then swap FK / IK / collision backend to `arm_kinematics`
- replace duplicated path-collision logic with a shared corrected helper

Why last:

- it depends most heavily on the current MoveIt-shaped IK interaction pattern
- it exercises both planning-time IK failure handling and path validation

## Twist Frame Swap

The twist-frame swap design from `report-v2.md` should stay.

If the goal is to avoid computing more frames than necessary in the real-time loop, then:

- detect frame changes on the subscription thread
- build the new FK tree there
- hand over the active tree to the RT thread through a wait-free handoff mechanism

One extra note should be added:

This design assumes `frame_id` resolves to arm-local frames that FK can produce. If support
for arbitrary external TF frames is required later, that is a separate problem and is not
solved by FK tree swapping alone.

## Recommended Migration Order

The report should end with a sharper critical path.

0. Preserve the existing controllers under `_old` names
   - copy `./arm_controllers` to `./arm_controllers_old`
   - rename each legacy controller package, plugin export, class export, and controller-facing
     plugin name with an `_old` suffix
   - add the legacy packages to the workspace Nix package set and ensure the bringup/runtime
     package closure still includes them
   - add or update `arm_bringup` controller parameter files and launch selection so the `_old`
     controllers are actually selectable from bringup
   - keep these legacy controllers loadable from `ros2_control` so bringup can explicitly fall
     back to `_old` while replacements are being developed
   - treat this as a compatibility branch in-tree, not as the preferred long-term architecture

1. Fix collision parity in `arm_kinematics`
   - zero/default-pose ACM generation
   - explicit allowed-pair configuration
   - pair-collection bug fix
   - structured setup diagnostics

2. Add `arm_kinematics::ros2_control` helpers
   - simple name construction
   - ordered ref acquisition
   - per-joint bundled acquisition

3. Migrate `nova_arm_controller`
   - validate collision parity
   - validate helper design

4. Migrate `nova_twistmapper`
   - position IK mode first
   - keep twist-frame tree swap

5. Refactor planner-facing IK API
   - MoveIt-shaped planner boundary replaced with explicit `expected` result

6. Migrate `nova_path_planner`
   - shared corrected path-collision helper
   - `arm_kinematics` FK / IK / collision backend

## Legacy Controller Preservation

Because other work may continue to depend on the current controller implementations while their
cleaner replacements are being built, the migration plan should include an explicit legacy
preservation step.

### Proposed approach

Create a parallel legacy tree:

- copy `./arm_controllers` to `./arm_controllers_old`
- rename each legacy controller package with an `_old` suffix
- rename each exported plugin class / pluginlib entry / controller-facing plugin name with an
  `_old` suffix
- keep the runtime-facing names explicit so `ros2_control` configs can choose between the new
  controller and the legacy `_old` controller without ambiguity


- package names in `package.xml`
- CMake project / install/export names where relevant
- plugin XML entries
- controller class names registered with pluginlib
- controller names used in configuration examples and launch files
- per-package `default.nix` packaging names and aggregated package-set entries

### Why this is worth doing

Benefits:

- replacement work can proceed without immediately breaking existing users
- bringup can intentionally fall back to a known controller implementation
- testing can compare old and new behavior side by side

### Constraints

This should be treated as a temporary compatibility mechanism, not as the target architecture.

The important requirement is not just "copy the sources." It is "make both versions loadable in
the same workspace without confusing pluginlib or `ros2_control`."

That means the `_old` suffix needs to be applied consistently to:

- package identity
- exported plugin identity
- controller class identity where required for export clarity

If this step is taken, it should happen before or alongside the first replacement controller so
that downstream users have a stable fallback path from the start.

## Workspace Packaging and Bringup Impact

Adding `arm_controllers_old` is not just a source-tree and pluginlib task. It also affects:

- workspace packaging through Nix
- `arm_bringup` launch scripts and controller parameter files
- other configs that hardcode controller names

The migration plan should say this explicitly.

### Nix workspace packaging

If legacy controllers are kept under `_old` names, they also need to remain part of the
workspace package set so they are built and discoverable.

This implies at least:

- add `./arm_controllers_old` to the workspace package aggregation
- give each legacy package a distinct Nix attribute / package name
- ensure `arm_bringup`'s runtime package closure still includes the legacy controller packages

One concrete issue already visible in the current tree:

- the root `default.nix` does not currently import `./arm_controllers` at all

At the wider workspace level, the relevant package list lives in:

- `~/nixfiles/packages/ros/nova-workspace/default.nix`

That file is the team-package list used to build `pkgs.ros.nova-workspace`, so if legacy
controllers are expected to remain part of `nova-workspace`, the `_old` controller packages
also need to be threaded into that package set.

So the report should not just say "also add the old controllers to Nix." It should say:

- wire controller packages into the workspace package set properly
- then add the `_old` controller packages alongside the new ones

In practice, that likely means touching both:

- the local package aggregation under this repository
- `~/nixfiles/packages/ros/nova-workspace/default.nix`

### `arm_bringup` launch integration

`arm_bringup` currently hardcodes controller names in launch logic and parameter files.

Examples include:

- controller spawner arguments in `arm_bringup/launch/control.launch.py`
- controller spawner arguments in `arm_bringup/launch/path.control.launch.py`
- controller type mappings in `arm_bringup/params/new.controllers.yaml`
- controller type mappings in `arm_bringup/params/old.controllers.yaml`

If `_old` controllers are introduced, the plan should include:

- new or updated controller YAML files mapping `_old` runtime controller names to `_old`
  plugin types
- launch-level selection of old vs new controller sets
- explicit fallback behavior in bringup instead of relying on users to hand-edit parameter files

### Recommended `arm_bringup` shape

The cleanest approach is to make the fallback explicit in configuration.

For example:

- keep separate controller parameter files for new and legacy controller stacks
- add a launch argument or config choice that selects the legacy `_old` stack
- make spawner names consistent with whichever stack is selected

The important point is that "findable from the control launch script" means more than package
installation. It also means the launch and YAML wiring must name the legacy controllers
correctly.

### Secondary config consumers

The report should also note that some non-bringup configs hardcode controller names and may
need parallel `_old` variants or updated selection logic.

Examples in this tree include teleop / control-mode configuration that references:

- `nova_arm_velocity_controller`
- `nova_arm_position_controller`
- `nova_twistmapper`
- `nova_path_planner`

These are not the primary migration target, but they are part of the operational surface area.
If `_old` controllers are introduced and expected to be usable in practice, these config
consumers need to be audited as part of the fallback rollout.

## Final Position

`report-v2.md` was broadly correct about the destination, but it treated the remaining work
as mostly integration glue. That was the main mistake.

The better conclusion is:

- FK-side migration is architecturally ready
- ownership direction is good
- reusable controller helpers belong in `arm_kinematics`
- `nova_arm_controller` should go first

But:

- collision semantics parity is still a real core blocker
- `nova_path_planner` needs an API-shape refactor before backend migration
- velocity IK should be treated as plugin-dependent rather than co-equal by default
- the controller-helper story needs to cover bundled multi-interface acquisition, not just
  flat name lists

That is the path that gets the library genuinely ready to start integrating into different
`ros2_control` controllers without dragging MoveIt's assumptions forward.
