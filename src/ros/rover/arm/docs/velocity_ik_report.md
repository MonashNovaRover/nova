# `nova_twistmapper` Velocity IK Support Report

## Executive Summary

The missing hole is in `nova_twistmapper`, not in `arm_kinematics`.

`arm_kinematics::InverseKinematicsPlugin` already exposes both:

- `get_position_ik(...)` in `arm_kinematics/arm_kinematics/include/arm_kinematics/inverse/inverse_kinematics_plugin.hpp:117`
- `get_velocity_ik(...)` in `arm_kinematics/arm_kinematics/include/arm_kinematics/inverse/inverse_kinematics_plugin.hpp:139`

and the base class already provides a default velocity implementation in
`arm_kinematics/arm_kinematics/src/inverse/inverse_kinematics_plugin.cpp:66`.

The current `nova_twistmapper` controller is still hard-wired to:

- request `position` command interfaces only
- solve position IK only
- collision-check a joint-position path only
- seed command interfaces with current joint positions on activate

So velocity mode is currently unimplemented at the controller layer.

The most important integration detail is this:

For `get_velocity_ik(...)`, the `ik_seed_pose` must match `ik_seed_state` exactly
(`inverse_kinematics_plugin.hpp:129-130`). That means velocity mode cannot reuse
`twistmapper_pose_` as the seed pose. It must run FK on the current joint state every update
and use the actual end-effector pose as the velocity IK seed.

## Relevant Findings

### 1. `arm_kinematics` already supports velocity IK

The API is already there:

- `get_position_ik(...)`: `inverse_kinematics_plugin.hpp:117-120`
- `get_velocity_ik(...)`: `inverse_kinematics_plugin.hpp:139-144`

The default implementation is not a stub. It does real work:

1. applies the twist over `time_step` to the seed pose
2. calls `get_position_ik(...)` on that displaced pose
3. finite-differences the returned joint positions against the seed state

That behavior is implemented in
`arm_kinematics/arm_kinematics/src/inverse/inverse_kinematics_plugin.cpp:66-94`.

The package docs say the same thing explicitly in
`arm_kinematics/arm_kinematics/docs/implementation_guide.md:319-324`.

### 2. The current Banksia IK plugin inherits the default velocity behavior

`BanksiaIKPlugin` overrides `get_position_ik(...)` only:

- `arm_kinematics/arm_kinematics/src/plugins/inverse/banksia_ik_plugin.cpp:27-149`

It does not override `get_velocity_ik(...)`, so today it would use the base-class fallback.

That means there is no library blocker for an initial velocity mode in twistmapper. The first
cut would be plugin-dependent velocity IK built on the finite-difference fallback. If an
analytical velocity override is added later, the controller API does not need to change.

### 3. `nova_twistmapper` is currently position-only by construction

The controller currently declares:

- position command interfaces only:
  `arm_controllers/nova_twistmapper/src/nova_twistmapper.cpp:61-69`
- position state interfaces only:
  `arm_controllers/nova_twistmapper/src/nova_twistmapper.cpp:72-80`

Its per-joint handle stores only:

- a position state ref
- a single command ref

See `arm_controllers/nova_twistmapper/include/nova_twistmapper/nova_twistmapper.hpp:87-92`.

At runtime it:

1. reads current joint positions
2. integrates the input twist into a candidate target pose
3. calls `get_position_ik(...)`
4. path-collision-checks from current joints to solved joints
5. writes the solved joint positions to the command interfaces

That flow is in `arm_controllers/nova_twistmapper/src/nova_twistmapper.cpp:180-244`.

So the current controller is not "missing an IK backend call". It is missing a full alternate
runtime mode.

### 4. The parameter schema has no mode selector today

`arm_controllers/nova_twistmapper/src/nova_twistmapper_parameter.yaml:1-63`
contains no parameter for:

- position-vs-velocity command output
- position-vs-velocity IK selection

There is already a precedent elsewhere in the repo:

- `nova_arm_controller` uses `use_position_control`
  in `arm_controllers/nova_arm_controller/src/nova_arm_controller_parameter.yaml:46-50`
- that parameter switches the command interface type in
  `arm_controllers/nova_arm_controller/src/nova_arm_controller.cpp:33-35`
  and `:60-67`

### 5. There is already helper support for velocity command interfaces

`arm_kinematics::ros2_control::command_interface_names(...)` is generic over the command type:

- `arm_kinematics/arm_kinematics/include/arm_kinematics/ros2_control/interface_names.hpp:66-89`

There is also explicit helper coverage for per-joint bundles with velocity commands and optional
state interfaces:

- `arm_kinematics/arm_kinematics/include/arm_kinematics/ros2_control/joint_interface_bundle.hpp:27-74`
- tests in
  `arm_kinematics/arm_kinematics/test/ros2_control/test_joint_interface_bundle.cpp:195-212`

That said, twistmapper does not strictly need the bundle helper for this change. Its current
state requirements remain simple enough that parameterizing the existing command lookup is
likely the lowest-risk path.

The more important structural question is not the lookup helper. It is whether the controller
keeps one flat bag of members with mode checks everywhere, or makes the mode-specific runtime
state explicit. The latter is cleaner.

## What Must Change In `nova_twistmapper`

### 1. Add a controller mode parameter

There are two separate design questions here:

1. how the mode is exposed as a parameter
2. how the mode is represented internally

For the parameter, there are two reasonable shapes:

- a boolean matching repo precedent, for example `use_position_control`
- a string mode, for example `command_mode: "position" | "velocity"`

For the parameter, my recommendation is still the boolean if that is the path of least
resistance in `generate_parameter_library`.

Reason:

- it matches `nova_arm_controller`
- it is the least disruptive option in `generate_parameter_library`
- this controller currently only has two meaningful output modes anyway

But that recommendation only applies to the parameter surface.

Internally, a raw boolean is not the clean design. It invites `if (use_position_control)` checks
to spread across lifecycle, interface lookup, update logic, and halting. The controller should
convert the parameter once into an explicit internal mode, for example:

```cpp
enum class TwistmapperMode {
  Position,
  Velocity,
};
```

If a string parameter is practical, that is cleaner at the configuration boundary too. If not,
keep the bool externally and the enum internally.

Suggested behavior:

- `true`: current behavior, position commands via `get_position_ik(...)`
- `false`: new behavior, velocity commands via `get_velocity_ik(...)`

Default should remain the current behavior to preserve compatibility.

### 2. Switch command interface configuration by mode

Today twistmapper hardcodes `hardware_interface::HW_IF_POSITION` in:

- `command_interface_configuration()`: `nova_twistmapper.cpp:61-69`
- `configure_joints()`: `nova_twistmapper.cpp:429-435`

That needs to become mode-dependent.

State interfaces do not need the same mode switch.

For both modes, twistmapper still needs current joint positions because:

- position IK needs the seed joint state
- velocity IK needs the seed joint state
- FK for twist-frame resolution needs joint positions
- collision checking still operates in joint-position space

So position state should stay required in both modes.

### 3. Make mode-specific runtime state explicit

The proposed behavior difference is not just "call a different IK function".

Position mode and velocity mode have different control-state semantics:

- in position mode, the persistent target pose is part of the control loop
- in velocity mode, the current EE pose from FK is the IK seed, while any target pose is only
  a derived/debug quantity

If the controller keeps one flat set of members and branches ad hoc in `update()`,
`on_activate()`, and `halt()`, it will work, but it will not be clean code.

The better structure is:

- shared controller resources remain shared
  - plugin loader / FK / IK / collision manager
  - joint handles
  - subscription and twist-frame plumbing
  - common scratch buffers where the semantics are identical
- mode-specific persistent runtime state is represented explicitly

Concretely, this wants small mode structs and a variant, for example:

```cpp
enum class TwistmapperMode {
  Position,
  Velocity,
};

struct PositionRuntime {
  Eigen::Isometry3d target_pose{Eigen::Isometry3d::Identity()};
  std::vector<double> solution_positions{};
};

struct VelocityRuntime {
  Eigen::Isometry3d visualized_target_pose{Eigen::Isometry3d::Identity()};
  std::vector<double> solution_velocities{};
  std::vector<double> predicted_next_positions{};
};

using ModeRuntime = std::variant<PositionRuntime, VelocityRuntime>;
```

The exact names can change. The point is that the type system should help prevent mixing:

- the virtual target pose used for visualization/debugging
- the actual EE seed pose used for velocity IK
- the position-command output buffer
- the velocity-command output buffer

This is the cleanest way to stop position-mode assumptions leaking into velocity mode.

### 4. Split "compute base twist" from "integrate target pose"

Right now `integrate_twist(...)` does two jobs at once:

1. resolve the incoming `TwistStamped` into the base frame
2. integrate that twist into the controller's internal target pose

That is fine for position mode, but awkward for velocity mode.

Velocity mode still needs the base-frame twist, but the IK call should consume the twist
directly rather than a synthesized target pose.

The clean refactor is:

- keep a helper that resolves the incoming twist into base-frame `arm_kinematics::Twistd`
- keep target-pose integration as a separate step used for TF/debug state

That preserves the current visualization behavior without forcing velocity mode through the
position-IK path.

### 5. Velocity mode must use current EE pose from FK, not `twistmapper_pose_`

This is the subtle part that matters.

The API contract says:

- `ik_seed_pose` must match `ik_seed_state`, otherwise the solution will be wrong
  (`inverse_kinematics_plugin.hpp:129-130`)

But `twistmapper_pose_` is not the current EE pose. In the current controller it is a virtual
target pose:

- initialized from EE FK on activate:
  `nova_twistmapper.cpp:472-474`
- then advanced each update by integrating the incoming twist:
  `nova_twistmapper.cpp:182`
- and only updated after a command is accepted:
  `nova_twistmapper.cpp:243-244`

That makes it the right object for "desired target pose" semantics, but the wrong object to pass
as `ik_seed_pose` in velocity mode.

Velocity mode therefore needs to do this every update:

1. read current joint positions
2. run `kinematics_->ee_tree->position_fk(current_joint_state_values_, fk_pose_buffer_)`
3. use `fk_pose_buffer_.front()` as `ik_seed_pose`
4. call `get_velocity_ik(base_twist, current_ee_pose, current_joint_state_values_, ...)`

The good news is that twistmapper already owns `kinematics_->ee_tree`
(`nova_twistmapper.hpp:94-101`) and already has `fk_pose_buffer_`
(`nova_twistmapper.hpp:140-142`), so this is a controller-flow change, not a library change.

### 6. Collision checking in velocity mode still needs a predicted joint-position path

`check_path_collision(...)` works between two joint-position states:

- declaration:
  `arm_kinematics/arm_kinematics/include/arm_kinematics/collision/collision_manager.hpp:124-136`
- implementation:
  `arm_kinematics/arm_kinematics/src/collision/collision_manager.cpp:136-176`

So velocity mode cannot pass the raw joint velocities straight into collision checking.

It should instead compute:

- `predicted_next_joint_positions[i] = current_joint_state_values_[i] + joint_velocities[i] * dt`

and then call `check_path_collision(...)` on:

- start = current joint positions
- end = predicted next joint positions

That keeps the current self-intersection semantics aligned with the existing controller:
the command is rejected if the next commanded motion segment would self-intersect.

This is the right minimum behavior, but it is still a discrete approximation. It is sensitive
to the controller period because the predicted end state is `q + qdot * dt`. That is acceptable
for a first implementation, but the code should treat `dt` as an explicit input to validate, not
as an invisible constant.

### 7. Activate/deactivate/halt behavior must diverge by mode

This is another easy place to get it wrong.

Today:

- on activate, twistmapper seeds each command interface with the current joint position
  `nova_twistmapper.cpp:476-478`
- `halt()` only injects a zero `TwistStamped`
  `nova_twistmapper.cpp:547-552`

That is acceptable enough for position commands, because "hold current position" is a valid
safe default.

It is not acceptable for velocity commands.

In velocity mode:

- on activate, command interfaces should be explicitly set to `0.0`
- on deactivate and inactive halt paths, command interfaces should be explicitly set to `0.0`

Just stashing a zero twist message is not enough, because that does not itself write zero
velocity commands to the hardware/chained interfaces.

`nova_arm_controller` already has the right precedent here:

- position mode seeds current positions on activate
- velocity mode does not
- halt writes `0.0`

See:

- `arm_controllers/nova_arm_controller/src/nova_arm_controller.cpp:33-35`
- `arm_controllers/nova_arm_controller/src/nova_arm_controller.cpp:499-507`
- `arm_controllers/nova_arm_controller/src/nova_arm_controller.cpp:574-578`

## Recommended Runtime Design

This is the lowest-risk design I found.

It is not enough on its own to make the implementation clean. The mode split should be explicit
in the code structure too.

### Position mode

Leave the current behavior mostly intact:

1. read current joint positions
2. resolve incoming twist into base frame
3. integrate that twist into `candidate_pose`
4. call `get_position_ik(candidate_pose, current_joint_state_values_, solution_positions)`
5. collision-check `current_joint_state_values_ -> solution_positions`
6. write position commands
7. update `twistmapper_pose_ = candidate_pose`

### Velocity mode

Use the same front half, but branch after twist resolution:

1. read current joint positions
2. resolve incoming twist into base frame
3. run EE FK on current joint positions to get `current_ee_pose`
4. call
   `get_velocity_ik(base_twist, current_ee_pose, current_joint_state_values_, solution_velocities, dt)`
5. compute `predicted_next_joint_positions = current + solution_velocities * dt`
6. collision-check `current_joint_state_values_ -> predicted_next_joint_positions`
7. write velocity commands
8. independently integrate `twistmapper_pose_` for TF/debug target broadcasting

The important separation is:

- `current_ee_pose` is for the IK seed
- `twistmapper_pose_` remains the controller's desired target/debug pose

Do not collapse those into one variable in velocity mode.

### Recommended code shape

The update logic should not become one large `if (mode == ...)` function with different local
variables and half-shared semantics.

The cleaner shape is:

1. a small helper to convert the parameter to `TwistmapperMode`
2. shared helpers for:
   - reading current joint positions
   - resolving incoming twist into base frame
   - FK of named frames
   - collision checking
3. two focused mode-specific update helpers
   - `update_position_mode(...)`
   - `update_velocity_mode(...)`
4. explicit mode-specific runtime state stored in a variant

That gives the controller one shared outer shell and two small inner control laws, which is the
right structure for this feature.

## Behavior and Risk Notes

### 1. The default velocity IK path inherits position-IK branch behavior

Because the default `get_velocity_ik(...)` is implemented by calling `get_position_ik(...)` on
a pose displaced by `dt` and then finite-differencing the result, it inherits the same branch
selection and discontinuity behavior as the position IK path.

That is not a blocker. It just means:

- the initial velocity mode is acceptable as a controller feature
- it should be described as plugin-dependent
- an analytical override would improve behavior near branch boundaries and singularities

### 2. Buffer sizing is already compatible

The current controller preallocates `ik_solution_` to `joint_names.size()` in
`nova_twistmapper.cpp:343-345`.

The current Banksia position IK requires the output buffer to already have space for at least
6 joints (`banksia_ik_plugin.cpp:36-39`), so the existing "preallocate once, reuse in update"
pattern is already the right shape for both modes.

### 3. Position state remains mandatory

Velocity mode does not remove the need for joint positions.

The controller still needs joint positions for:

- FK of the twist frame
- FK of the end effector seed pose
- IK seed state
- predicted-next-state collision checking

So there is no good reason to make position state optional for this feature.

### 4. A flat bool-driven implementation would work, but it would age badly

The minimum possible implementation is:

- add `use_position_control`
- branch in `command_interface_configuration()`
- branch in `configure_joints()`
- branch in `update()`
- branch in `on_activate()` and `halt()`

That will function, but it is not the code I would want to maintain. The risk is not immediate
correctness. The risk is semantic drift over time, where:

- one branch starts using the wrong pose variable
- lifecycle behavior diverges silently
- a later feature gets added to only one path

That is exactly why the mode-specific state should be explicit.

## Suggested Implementation Checklist

1. Add a mode parameter to `nova_twistmapper_parameter.yaml`, defaulting to current behavior.
2. Convert that parameter once into an explicit internal `TwistmapperMode`.
3. Add explicit mode-specific runtime structs and store them in a variant.
4. Add a small helper similar to `joint_command_type()` in `nova_arm_controller`.
5. Switch `command_interface_configuration()` to use that helper.
6. Switch `configure_joints()` command lookup to use that helper.
7. Refactor `integrate_twist(...)` so base-twist resolution is reusable by both modes.
8. Split the control law into position-command and velocity-command update helpers.
9. In velocity mode, compute `current_ee_pose` from FK each update before calling `get_velocity_ik(...)`.
10. In velocity mode, collision-check against `current + qdot * dt`.
11. Make `on_activate()` and `halt()` mode-aware:
   hold current position in position mode, write zeros in velocity mode.
12. Keep TF target-pose broadcasting in both modes, but do not use that target pose as the
    velocity IK seed.

## Test Coverage Worth Adding

At minimum:

1. Parameter/configuration test: position mode requests `.../position` command interfaces, velocity mode requests `.../velocity`.
2. Update-path test: position mode calls `get_position_ik(...)`, velocity mode calls `get_velocity_ik(...)`.
3. Seed-pose correctness test: velocity mode uses EE FK of the current joint state, not `twistmapper_pose_`.
4. Collision-path test: velocity mode collision-checks `current -> current + qdot * dt`.
5. Lifecycle test: velocity mode writes zero commands on halt/deactivate.
6. Regression test: position mode behavior is unchanged.

## Bottom Line

The required library support already exists.

The real work is in `nova_twistmapper`:

- add a mode parameter
- represent the mode explicitly in code, rather than as a raw bool threaded everywhere
- switch command interface type by mode
- use `get_velocity_ik(...)` when configured
- seed velocity IK from current EE FK, not from the virtual target pose
- collision-check the predicted next joint positions
- make activate/halt semantics safe for velocity commands

That is a moderate controller refactor, not a new kinematics-library feature.

If implemented with explicit mode state and focused mode-specific update helpers, it should be
good code. If implemented as a flat bool-driven branch soup, it will work, but it will not be
clean.
