# Controller Usage

This guide describes the public `arm_kinematics` structures that controllers usually work with
directly. The examples are based on the usage patterns in:

- `nova_twistmapper`
- `nova_path_planner`
- `nova_arm_controller`

## Standard Bring-Up Sequence

Most controllers follow this setup order:

1. Get a non-empty `robot_description`.
2. Construct `arm_kinematics::PluginLoader`.
3. Create the plugins the controller needs.
4. Build FK trees for the requested frames.
5. Build a `CollisionManager` if self-collision checks are required.
6. Acquire ordered state and command interface refs.

This keeps all kinematics pieces tied to the same shared robot model and parameter set.

The main reason to follow this flow is that it front-loads expensive structural work and leaves
the controller loop with only prebuilt objects and ordered numeric vectors.

## 1. Create the Loader and Plugins

`PluginLoader` is the normal controller entry point. It owns the shared robot model and loads the
configured plugin implementations.

```cpp
const std::string robot_description = get_robot_description();
arm_kinematics::PluginLoader loader{*get_node(), robot_description};

auto fk = loader.make_fk();
if (!fk) {
  RCLCPP_ERROR(logger, "Failed to create FK plugin.");
  return CallbackReturn::ERROR;
}

auto ik = loader.make_ik();
if (!ik) {
  RCLCPP_ERROR(logger, "Failed to create IK plugin.");
  return CallbackReturn::ERROR;
}
```

Use only the plugins you need. `nova_arm_controller`, for example, only builds collision support
when collision limiting is enabled.

Why use `PluginLoader` instead of constructing pieces manually:

- it keeps FK, IK, and collision plugins on the same `RobotModel`
- it centralizes parameter-driven plugin selection
- it avoids subtle drift between subsystems that should share one robot description

## 2. Build FK Trees

Controllers typically build a `ForwardKinematicsPlugin::Tree` for a specific frame they want to
track, such as the end effector or an input twist frame.

The usual input description is a vector of `NamedStateInterfaceDefinition` in the same order as
the controller's joint state vector.

```cpp
std::vector<arm_kinematics::NamedStateInterfaceDefinition> inputs;
inputs.reserve(params_.joint_names.size());
for (const auto & joint_name : params_.joint_names) {
  inputs.emplace_back(joint_name, arm_kinematics::InterfaceId::Position());
}

auto tree_result = fk->make_tree(
  arm_kinematics::span<const arm_kinematics::NamedStateInterfaceDefinition>(
    inputs.data(), inputs.size()),
  params_.base_link_name,
  arm_kinematics::FrameDefinitions{params_.ee_link_name});

if (!tree_result) {
  RCLCPP_ERROR(logger, "Failed to build FK tree: %s", tree_result.error().format().c_str());
  return CallbackReturn::ERROR;
}

auto tree = arm_kinematics::ForwardKinematicsPlugin::Tree::SharedPtr(
  std::move(tree_result.value().tree));
```

At runtime:

```cpp
tree->position_fk(current_joint_state_values_, fk_pose_buffer_);
const Eigen::Isometry3d & ee_pose = fk_pose_buffer_.front();
```

Why use a tree instead of asking the FK plugin to compute one pose ad hoc each time:

- the tree bakes in frame reduction and joint mapping once
- the runtime call becomes a pure ordered-vector to ordered-pose update
- the controller can reuse preallocated output buffers

### Ordering contract

- The joint state vector passed to `position_fk(...)` must be in the same order as the inputs used
  to build the tree.
- The output pose vector must already be sized correctly.
- If you request multiple frames, `make_tree(...)` may reorder them for compute efficiency.
  `MakeTreeResult::frame_order` tells you how to map between requested order and returned order.

## 3. Use IK

IK plugins solve either a target pose or a target twist into ordered joint outputs.

Position IK:

```cpp
auto ik_result = ik->get_position_ik(
  candidate_pose,
  current_joint_state_values_,
  solution_positions);

if (!ik_result) {
  RCLCPP_WARN(logger, "IK failed: %s", ik_result.error().format().c_str());
}
```

Velocity IK uses the same ordering contract, but requires both the current end-effector pose and a
non-zero time step.

The seed state and output vector are expected to follow the controller's joint order.

Why use IK this way:

- task-space controllers can stay focused on poses or twists
- the plugin hides solver-specific details
- the seed state gives the solver continuity and helps select the nearest useful solution

## 4. Build and Query Collision Support

If the controller needs self-collision checking, build a `CollisionManager` once during setup.

```cpp
auto collision_config = arm_kinematics::read_collision_config(
  get_node()->get_node_parameters_interface());

auto collision_result = arm_kinematics::make_collision_manager(
  loader,
  fk,
  params_.joint_names,
  collision_config);

if (!collision_result) {
  RCLCPP_ERROR(
    logger,
    "Failed to build collision manager: %s",
    collision_result.error().format().c_str());
  return CallbackReturn::ERROR;
}

arm_kinematics::CollisionManager collision_manager = std::move(*collision_result);
```

Why use `CollisionManager` instead of wiring FK and collision plugins separately in each caller:

- it packages the required FK tree and collision backend together
- it makes the runtime sequence explicit: update poses first, then query
- it reduces the chance of forgetting to keep collider poses in sync with the candidate state

At runtime:

```cpp
collision_manager.update_poses(candidate_joint_positions);
if (collision_manager.collide()) {
  // reject or modify the candidate command
}
```

For interpolated path checks:

```cpp
auto collision_result = arm_kinematics::check_path_collision(
  collision_manager,
  start_state,
  end_state,
  step_size,
  scratch);
```

### Ordering contract

- The joint state vector passed to `update_poses(...)` or `check_path_collision(...)` must be in
  the same joint order used to construct the collision manager.
- `parent_link_names()` returns collider parent links in the manager's internal collider order,
  which is useful for debugging but should not be treated as a controller command order.

### Collision footguns

- Calling `collide()` without updating poses first
  The result describes the last poses pushed into the manager, not an implied current controller
  state.
- Treating collision setup as cheap
  Building the manager can create FK structures, filter colliders, and initialize a backend.
- Using different joint orderings for command generation and collision checks
  This usually fails silently by checking the wrong physical configuration.

## 5. Acquire ROS 2 Control Interface Refs

The ROS 2 control helpers keep controller interface declaration and handle acquisition aligned.

Declare state interface names:

```cpp
std::vector<arm_kinematics::NamedStateInterfaceDefinition> state_defs;
for (const auto & joint_name : params_.joint_names) {
  state_defs.emplace_back(joint_name, arm_kinematics::InterfaceId::Position());
}

const auto state_names = arm_kinematics::ros2_control::state_interface_names(
  arm_kinematics::span<const arm_kinematics::NamedStateInterfaceDefinition>(
    state_defs.data(), state_defs.size()));
```

Acquire ordered state refs:

```cpp
auto state_refs_result = arm_kinematics::ros2_control::find_state_interface_refs(
  state_interfaces_,
  arm_kinematics::span<const arm_kinematics::NamedStateInterfaceDefinition>(
    state_defs.data(), state_defs.size()));
```

Build command interface names and acquire refs:

```cpp
const auto command_names = arm_kinematics::ros2_control::command_interface_names(
  params_.joint_names,
  hardware_interface::HW_IF_POSITION,
  params_.chained_controller_name);

auto command_refs_result = arm_kinematics::ros2_control::find_command_interface_refs(
  command_interfaces_,
  arm_kinematics::span<const std::string>(command_names.data(), command_names.size()));
```

### Ordering contract

- Name helpers preserve declaration order.
- Ref lookup helpers return refs in the same order as the requested definitions or names.
- That order should match the vectors your controller later reads from or writes to.

Why use the helpers:

- they keep interface naming logic out of controller glue code
- they make ordering explicit
- they report missing interfaces in batch instead of failing one lookup at a time

## Recommended Usage Rules

- Build plugins, FK trees, collision managers, and joint maps during configuration or activation,
  not in `update()`.
- Treat joint ordering as part of the API contract. Most mistakes in controller integration come
  from mismatched vector order, not from failed math.
- Prefer `NamedStateInterfaceDefinition` at controller boundaries. Use the lower-level
  `StateInterfaceDefinition` APIs only when you are already working in analysis/building code.
- Reuse plugin instances and preallocated output buffers across updates.
- Null-check plugin pointers returned by `PluginLoader`. Loading or initialization failures are
  reported by returning `nullptr`, not by throwing.
