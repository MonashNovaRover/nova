# Integrating arm_kinematics with ros2_control Controllers

## Problem Statement

Three arm controllers (`nova_arm_controller`, `nova_twistmapper`, `nova_path_planner`) depend on
MoveIt2 for kinematics and collision checking. This dependency brings:

- **~300 lines of duplicated MoveIt setup code** between twistmapper and path_planner (URDF
  parsing, SRDF fallback generation, PlanningScene creation, allowed collision matrix computation,
  kinematics solver loading, compat node creation)
- **A heavyweight transitive dependency** (moveit_core, moveit_ros_planning, srdfdom, etc.)
- **Impedance mismatch** with arm_kinematics, which is purpose-built for this robot and designed
  for real-time-safe operation

arm_kinematics already provides the tools to replace every MoveIt usage in these controllers:
FK, IK, collision checking, joint value mapping, and URDF-derived robot modelling. What's missing
is the glue between arm_kinematics and ros2_control's interface system, and a clear pattern for
how controllers should use the library.

---

## Current State of the Controllers

### Duplicated code

| Duplicated block | Controllers | ~Lines each |
|---|---|---|
| URDF parsing + `get_robot_description()` fallback | twistmapper, path_planner | 15 |
| `construct_srdf_fallback_string()` | twistmapper, path_planner | 30 |
| MoveIt RobotModel + PlanningScene creation | twistmapper, path_planner | 10 |
| `generate_allowed_collision_matrix()` | twistmapper, path_planner, arm_controller (via SelfCollisionLimiter) | 25 |
| `check_pose_for_self_intersection()` | twistmapper, path_planner | 30 |
| `check_path_for_self_intersection()` | twistmapper, path_planner | 25 |
| Kinematics solver loading + `ClassLoader` setup | twistmapper, path_planner | 30 |
| `create_compat_node_from_lifecycle()` | twistmapper, path_planner | 10 |
| Joint handle reordering to match MoveIt joint group order | twistmapper, path_planner | 15 |
| `configure_joints()` — `find_if` over state + command interfaces | all three | 30-60 |
| `state_interface_configuration()` — string concat loop | all three | 5-10 |
| `command_interface_configuration()` — string concat loop | all three | 5-10 |
| `get_state_pos_values()` | twistmapper, path_planner | 5 |
| `joint_to_command_interface_name()` (chained prefix logic) | twistmapper, path_planner | 5 |
| Verbose error logging on interface lookup failure | twistmapper, path_planner | 10 |

### The MoveIt dependency chain

Twistmapper and path_planner follow an identical setup pattern:

```
URDF string → urdf::parseURDF()
  → construct fallback SRDF (if none provided)
  → srdf::Model::initString()
  → moveit::core::RobotModel(urdf, srdf)
  → planning_scene::PlanningScene(robot_model)
  → generate_allowed_collision_matrix()          // check default pose, allow those pairs
  → create_compat_node_from_lifecycle()          // non-lifecycle node needed by MoveIt solver
  → pluginlib::ClassLoader<kinematics::KinematicsBase>
  → kinematics_solver_->initialize(compat_node, robot_model, joint_group, ...)
```

Every step here has a direct arm_kinematics equivalent (see next section).

### The joint handle reordering hack

Both twistmapper and path_planner reorder their `registered_joint_handles_` after acquisition
to match MoveIt's joint group ordering:

```cpp
// nova_twistmapper.cpp:416-423
std::unordered_map<std::string, size_t> joint_name_to_index;
for (size_t i = 0; i < joint_group_names.size(); ++i)
  joint_name_to_index[joint_group_names[i]] = i;

std::sort(registered_joint_handles_.begin(), registered_joint_handles_.end(),
  [&](const JointHandle& a, const JointHandle& b) {
    return joint_name_to_index[a.name] < joint_name_to_index[b.name];
  });
```

This exists because MoveIt FK/IK expects joint values in its joint group order, which may
differ from the controller's parameter order. This is exactly what `JointMap` solves — the
controller declares its input order, and the joint map produces the output order the FK tree
needs. With arm_kinematics, this hack disappears entirely.

### Divergent interface acquisition

The three controllers search `state_interfaces_` with different strategies:

```cpp
// nova_arm_controller.cpp:571 — matches prefix + interface name separately
const auto pos_state_handle = std::find_if(
    state_interfaces_.cbegin(), state_interfaces_.cend(),
    [&joint_name](const auto &interface) {
      return interface.get_prefix_name() == joint_name &&
             interface.get_interface_name() == HW_IF_POSITION;
    });

// nova_twistmapper.cpp:589 — constructs full name, matches get_name()
const auto state_interface_name = joint_name + "/" + hardware_interface::HW_IF_POSITION;
const auto state_handle = std::find_if(
    state_interfaces_.begin(), state_interfaces_.end(),
    [&state_interface_name](const auto &interface) {
      return interface.get_name() == state_interface_name;
    });
```

Command interface acquisition follows the same divergent patterns. Error logging on lookup
failure is copy-pasted verbatim between twistmapper and path_planner (dumping the entire
interface list with prefix/name/full-name).

### `get_value()` deprecation

All controllers use the deprecated `get_value()` API:

```cpp
joint_handle.state_pos.get().get_value()  // deprecated, returns NaN on failure
```

The modern API is `get_optional<double>()`, which returns `std::optional<double>`.

---

## What arm_kinematics Already Provides

Each of these replaces a chunk of the MoveIt machinery above.

### `RobotModel` → replaces URDF parsing + MoveIt robot model

```cpp
// arm_kinematics/common/robot_model.hpp
class RobotModel {
  explicit RobotModel(std::string robot_description);

  const urdf::Model & get_urdf_model() const;                        // lazy
  const TransmissionAnalysis & get_default_transmission_analysis() const;  // lazy
  const AnalysisTree & get_analysis_tree() const;                     // lazy
};
```

Replaces: `urdf::parseURDF()`, `srdf::Model`, `moveit::core::RobotModel`. No SRDF needed —
arm_kinematics derives transmission and mimic relationships directly from the URDF.

### `PluginLoader` → replaces kinematics solver loading

```cpp
// arm_kinematics/plugin_loader.hpp
class PluginLoader {
  explicit PluginLoader(PluginLoaderNodeInterfaces node, std::string robot_description);

  ForwardKinematicsPlugin::SharedPtr make_fk();
  InverseKinematicsPlugin::SharedPtr make_ik();
  MakeCollisionResult make_collision(
    const std::vector<std::string> & joint_names,
    const ForwardKinematicsPlugin::SharedPtr & fk);

  const RobotModel & get_robot_model() const;
};
```

Replaces: `pluginlib::ClassLoader<kinematics::KinematicsBase>`, solver loading, compat node
creation, solver initialization. Plugin types are read from ROS parameters
(`kinematics.forward_kinematics_plugin`, etc.).

### `ForwardKinematicsPlugin::make_tree()` → replaces MoveIt FK

```cpp
// arm_kinematics/forward/forward_kinematics_plugin.hpp
tl::expected<MakeTreeResult, MakeTreeError> make_tree(
  span<const NamedStateInterfaceDefinition> named_input_state_interfaces,
  const std::string & base_link_name,
  const FrameDefinitions & frames);
```

Returns a `Tree` whose `position_fk(joint_states, link_poses)` is the runtime FK call.
The `JointMap` is built internally — the controller's input order is the order declared in
`named_input_state_interfaces`. No reordering hack needed.

Replaces: `kinematics_solver_->getPositionFK(...)`, the joint handle reordering hack.

### `InverseKinematicsPlugin` → replaces MoveIt IK plugins

```cpp
// arm_kinematics/inverse/inverse_kinematics_plugin.hpp
bool get_position_ik(
  const Eigen::Isometry3d & ik_pose,
  const std::vector<double> & ik_seed_state,
  std::vector<double> & solution_state) const;

bool get_velocity_ik(
  const Twistd & ik_twist,
  const Eigen::Isometry3d & ik_seed_pose,
  const std::vector<double> & ik_seed_state,
  std::vector<double> & solution_velocities,
  double time_step) const;
```

Replaces: `kinematics_solver_->getPositionIK(...)`, `kinematics_solver_->searchPositionIK(...)`.
Works directly with `Eigen::Isometry3d` instead of `geometry_msgs::msg::Pose`.

Also provides `get_velocity_ik()` — the twistmapper could use this directly instead of
integrating a twist into a pose and then solving position IK. Both approaches should be
supported (see Design Decisions).

### `CollisionManager` → replaces MoveIt PlanningScene + checkSelfCollision

```cpp
// arm_kinematics/collision/collision_manager.hpp
struct CollisionManager {
  CollisionManager(
    ForwardKinematicsPlugin::Tree::SharedPtr tree,
    DiscreteCollisionPlugin::SharedPtr plugin);

  void update_poses(const std::vector<double> & joint_states);
  bool collide() const;
};

// Factory function
tl::expected<CollisionManager, const char *> make_collision_manager(
  PluginLoader & loader,
  const ForwardKinematicsPlugin::SharedPtr & fk,
  const std::vector<std::string> & joint_names);
```

Replaces: `planning_scene::PlanningScene`, `checkSelfCollision()`,
`generate_allowed_collision_matrix()`, `construct_srdf_fallback_string()`.

How the `AllowedCollisionMatrix` is populated by the convenience overload is an open question
(see Open Questions #4).

### `JointMap` → replaces the reordering hack

```cpp
// arm_kinematics/joint_map/joint_map.hpp
class JointMap {
  void map(span<const double> inputs, span<double> outputs) const;
};
```

Built internally by `make_tree()`. The controller declares its input state interfaces in
whatever order it likes. `JointMap::map()` reorders, duplicates, and applies mimic/affine
transformations to produce the output order the FK tree expects.

Replaces: the `std::sort(registered_joint_handles_, ...)` hack.

---

## Controller Lifecycle Pattern

### Worked example

```cpp
class ExampleIKController : public controller_interface::ControllerInterface {
  // -- arm_kinematics resources (owned) --
  arm_kinematics::PluginLoader plugin_loader_;
  arm_kinematics::ForwardKinematicsPlugin::SharedPtr fk_plugin_;
  arm_kinematics::ForwardKinematicsPlugin::Tree::UniquePtr fk_tree_;
  arm_kinematics::InverseKinematicsPlugin::SharedPtr ik_plugin_;
  arm_kinematics::CollisionManager collision_manager_;

  // Built in on_configure() from params. Reused for: InterfaceConfiguration,
  // make_tree(), and ref acquisition in on_activate().
  std::vector<arm_kinematics::NamedStateInterfaceDefinition> state_inputs_;

  // -- Resolved at activate time --
  std::vector<std::reference_wrapper<const hardware_interface::LoanedStateInterface>> state_refs_;
  std::vector<std::reference_wrapper<hardware_interface::LoanedCommandInterface>> command_refs_;

  // -- Preallocated runtime buffers --
  std::vector<double> input_buffer_;            // raw state interface values
  arm_kinematics::Isometry3dVector link_poses_; // FK output


  // ── on_init ──────────────────────────────────────────────────────
  CallbackReturn on_init() override {
    // Only create the param listener here. Params and all derived structures
    // are built in on_configure(), which is re-called on each reconfigure cycle.
    param_listener_ = std::make_shared<ParamListener>(get_node());
    return SUCCESS;
  }


  // ── on_configure ─────────────────────────────────────────────────
  CallbackReturn on_configure(const State &) override {
    // Re-read params (may have changed since last configure)
    params_ = param_listener_->get_params();

    // Build state input declarations from params.
    // These are reused for: InterfaceConfiguration, make_tree(), and ref acquisition.
    state_inputs_.clear();
    for (const auto & name : params_.joint_names)
      state_inputs_.emplace_back(name, arm_kinematics::InterfaceId("position"));

    // Create plugin loader (owns RobotModel internally).
    // Done here, not on_init, so it's rebuilt on reconfigure with fresh params.
    plugin_loader_ = arm_kinematics::PluginLoader(
      /* node interfaces */, get_robot_description());

    // Load plugins
    fk_plugin_ = plugin_loader_.make_fk();
    ik_plugin_ = plugin_loader_.make_ik();
    if (!fk_plugin_ || !ik_plugin_) return ERROR;

    // Build FK tree — JointMap is created internally
    auto tree_result = fk_plugin_->make_tree(
      state_inputs_,
      params_.base_link_name,
      /* FrameDefinitions for the links we need poses for */);

    if (!tree_result) {
      // tree_result.error() has structured diagnostics
      return ERROR;
    }
    fk_tree_ = std::move(tree_result->tree);

    // Build collision manager (creates its own FK tree for collider poses)
    auto collision_result = arm_kinematics::make_collision_manager(
      plugin_loader_, fk_plugin_, params_.joint_names);
    if (!collision_result) return ERROR;
    collision_manager_ = std::move(*collision_result);

    // Preallocate buffers
    input_buffer_.resize(state_inputs_.size());
    link_poses_.resize(/* frame count */);

    return SUCCESS;
  }


  // ── state_interface_configuration ────────────────────────────────
  InterfaceConfiguration state_interface_configuration() const override {
    // Reuse state_inputs_ to build the "joint/type" strings
    std::vector<std::string> names;
    names.reserve(state_inputs_.size());
    for (const auto & def : state_inputs_)
      names.push_back(def.joint_name + "/" + def.interface_id.name);

    // With a utility: return {INDIVIDUAL, state_interface_names(state_inputs_)};
    return {interface_configuration_type::INDIVIDUAL, names};
  }


  // ── on_activate ──────────────────────────────────────────────────
  CallbackReturn on_activate(const State &) override {
    // Resolve state_interfaces_ to ordered refs matching state_inputs_
    state_refs_.clear();
    state_refs_.reserve(state_inputs_.size());

    for (const auto & def : state_inputs_) {
      const auto expected_name = def.joint_name + "/" + def.interface_id.name;
      auto it = std::find_if(state_interfaces_.begin(), state_interfaces_.end(),
        [&](const auto & si) { return si.get_name() == expected_name; });

      if (it == state_interfaces_.end()) {
        RCLCPP_ERROR(get_node()->get_logger(),
          "Missing state interface: %s", expected_name.c_str());
        return ERROR;
      }
      state_refs_.emplace_back(*it);
    }

    // Similarly resolve command interfaces...

    return SUCCESS;
  }


  // ── update ───────────────────────────────────────────────────────
  return_type update(const rclcpp::Time & time, const rclcpp::Duration & period) override {
    // 1. Read state interface values into input buffer
    for (size_t i = 0; i < state_refs_.size(); ++i) {
      auto val = state_refs_[i].get().get_optional<double>();
      input_buffer_[i] = val.value_or(std::numeric_limits<double>::quiet_NaN());
    }

    // 2. FK — Tree applies JointMap internally, so input_buffer_ goes in directly
    fk_tree_->position_fk(input_buffer_, link_poses_);

    // 3. Collision check
    collision_manager_.update_poses(input_buffer_);
    if (collision_manager_.collide()) {
      // handle collision
    }

    // 4. IK (if this controller does IK)
    std::vector<double> ik_solution;
    ik_plugin_->get_position_ik(target_pose, input_buffer_, ik_solution);

    // 5. Write commands
    for (size_t i = 0; i < command_refs_.size(); ++i)
      command_refs_[i].get().set_value(ik_solution[i]);

    return OK;
  }
};
```

### What this replaces

| Controller concern | Before (MoveIt) | After (arm_kinematics) |
|---|---|---|
| URDF parsing | `urdf::parseURDF()` | `RobotModel` (via PluginLoader) |
| SRDF generation | `construct_srdf_fallback_string()` | Not needed |
| Robot model | `moveit::core::RobotModel(urdf, srdf)` | `RobotModel` |
| Planning scene | `planning_scene::PlanningScene(robot_model)` | Not needed |
| Collision matrix | `generate_allowed_collision_matrix()` | Built from URDF geometry |
| Self-collision | `checkSelfCollision()` | `CollisionManager::collide()` |
| FK | `kinematics_solver_->getPositionFK(...)` | `Tree::position_fk(...)` |
| IK | `kinematics_solver_->getPositionIK(...)` | `InverseKinematicsPlugin::get_position_ik(...)` |
| IK solver loading | `pluginlib::ClassLoader<KinematicsBase>` | `PluginLoader::make_ik()` |
| Compat node | `create_compat_node_from_lifecycle()` | Not needed |
| Joint reordering | `std::sort(handles, ...)` by MoveIt group order | `JointMap` (built by `make_tree()`) |

---

## Remaining Gaps

arm_kinematics provides the core kinematics functionality. What's still missing is the glue
between arm_kinematics and ros2_control's interface system.

### Gap 1: Interface name construction

Every controller manually concatenates `"joint_name/type"` strings for
`InterfaceConfiguration`. Both state and command sides have this, with command interfaces
additionally needing chained controller prefix logic.

**Current code (duplicated 3x for state, 3x for command):**

```cpp
// state_interface_configuration — all controllers
for (const auto & joint_name : params_.joint_names)
  conf_names.push_back(joint_name + "/" + HW_IF_POSITION);

// command with chained prefix — twistmapper, path_planner
std::string joint_to_command_interface_name(const std::string & joint_name) const {
  const auto prefix = params_.chained_controller_name.empty()
    ? "" : params_.chained_controller_name + "/";
  return prefix + joint_name + "/" + HW_IF_POSITION;
}
```

### Gap 2: Interface ref acquisition

Every controller searches `state_interfaces_` and `command_interfaces_` with `find_if` to
resolve declared names to `LoanedStateInterface` / `LoanedCommandInterface` refs. The search
strategy, error handling, and error logging diverge across controllers.

This work should happen once at activate time. The runtime update loop should iterate
pre-resolved refs only.

### Gap 3: Collision limiting

`SelfCollisionLimiter` (used by `nova_arm_controller`) is a `JointLimiterInterface` that
checks whether a desired joint state would cause self-collision. It currently depends on
MoveIt's `PlanningScene`. It needs to be rewritten to use arm_kinematics' `CollisionManager`.

This has implications for state interface acquisition: collision checking may require
**auxiliary joints** — joints the controller doesn't command but whose positions affect
collider geometry. These auxiliary joint values need to be sourced somewhere (see Design
Decisions).

### Gap 4: Auxiliary joint values

A controller may command 6 joints but collision checking may need the positions of 10 joints
(including passive or independently-controlled joints). The `JointMap` handles the mapping
from supplied inputs to required outputs, and will fail at build time if required outputs
cannot be derived. But the controller needs to know which auxiliary state interfaces to declare
and how to read them.

---

## Proposed Utilities

Two utilities. Both are thin, header-only, and live outside arm_kinematics — they are
ros2_control helpers that don't require arm_kinematics as a dependency for their primary
overloads.

### Utility A — Interface name construction

Generates the `"joint/type"` strings required by `InterfaceConfiguration::names`.

```cpp
namespace nova::ros2_control_utils {

// State interface names: ["shoulder_pitch/position", "elbow/position", ...]
std::vector<std::string> state_interface_names(
    span<const std::string>                 joint_names,
    std::initializer_list<std::string_view> types);

// Command interface names with optional chained controller prefix:
// ["nova_arm_controller/shoulder_pitch/position", ...]
std::vector<std::string> command_interface_names(
    span<const std::string>                 joint_names,
    std::string_view                        command_type,
    std::string_view                        chained_prefix = "");

} // namespace nova::ros2_control_utils
```

**arm_kinematics-aware overload** (separate header, depends on arm_kinematics):

```cpp
namespace arm_kinematics::ros2_control {

std::vector<std::string> state_interface_names(
    span<const NamedStateInterfaceDefinition> defs);

} // namespace arm_kinematics::ros2_control
```

This allows controllers that use `NamedStateInterfaceDefinition` (for `make_tree()`) to reuse
the same declarations for `InterfaceConfiguration` without restating the joint list.

### Utility B — Interface ref acquisition

Resolves declared interface names to ordered refs from the controller's `state_interfaces_` /
`command_interfaces_` vectors. Done once at activate time. Returns refs in the same order as
the input declarations, ready for indexed iteration at runtime.

```cpp
namespace nova::ros2_control_utils {

using LoanedStateRef = std::reference_wrapper<const hardware_interface::LoanedStateInterface>;
using LoanedCommandRef = std::reference_wrapper<hardware_interface::LoanedCommandInterface>;

struct MissingInterface {
    std::string expected_name;
};

// Resolve state interfaces by name.
// Returns refs[i] corresponding to names[i].
tl::expected<std::vector<LoanedStateRef>, std::vector<MissingInterface>>
find_state_interface_refs(
    std::vector<hardware_interface::LoanedStateInterface> & state_interfaces,
    span<const std::string>                                 names);

// Resolve command interfaces by name.
tl::expected<std::vector<LoanedCommandRef>, std::vector<MissingInterface>>
find_command_interface_refs(
    std::vector<hardware_interface::LoanedCommandInterface> & command_interfaces,
    span<const std::string>                                   names);

} // namespace nova::ros2_control_utils
```

**arm_kinematics-aware overload** (separate header):

```cpp
namespace arm_kinematics::ros2_control {

tl::expected<std::vector<LoanedStateRef>, std::vector<MissingInterface>>
find_state_interface_refs(
    std::vector<hardware_interface::LoanedStateInterface> & state_interfaces,
    span<const NamedStateInterfaceDefinition>                definitions);

} // namespace arm_kinematics::ros2_control
```

### Why no "value reading" utility

Reading values from resolved refs is a trivial loop:

```cpp
for (size_t i = 0; i < refs.size(); ++i) {
  auto val = refs[i].get().get_optional<double>();
  buffer[i] = val.value_or(std::numeric_limits<double>::quiet_NaN());
}
```

A separate utility for this would be over-engineering. The loop can be inlined wherever values
are read. The important thing is to use `get_optional<double>()` instead of the deprecated
`get_value()`.

---

## Design Decisions

### Each controller owns its own PluginLoader

For the initial integration, each controller that needs arm_kinematics creates its own
`PluginLoader` and owns its own `RobotModel`. This is the simplest ownership model and
matches how controllers work in ros2_control (independent lifecycle, no shared resources).

URDF parsing is cheap (~0.1ms). There is no performance reason to share.

**Future direction:** A shared kinematics component (lazy singleton or dedicated node) could
centralise `RobotModel`, plugin instances, and parameterisation. Individual controllers could
deviate from the shared configuration for special cases. But this optimisation should wait
until there's a demonstrated need.

### Twistmapper supports both position IK and velocity IK

arm_kinematics' `InverseKinematicsPlugin` provides both:

- `get_position_ik(pose, seed, solution)` — current approach: integrate twist into a target
  pose, solve position IK each tick
- `get_velocity_ik(twist, seed_pose, seed_state, velocities, dt)` — alternative: convert
  twist directly to joint velocities, no intermediate pose

The twistmapper should support both via a parameter. Position IK is proven and the intermediate
pose is useful for TF broadcasting and path validation. Velocity IK is mathematically cleaner
and avoids pose drift. The choice depends on the use case.

### Collision limiting — no JointLimiterInterface

The current `SelfCollisionLimiter` inherits from `joint_limits::JointLimiterInterface`. That
interface was designed for separable per-joint limits (position bounds, velocity bounds); it's
a poor fit for collision, which is non-separable across joints and requires FK.

The replacement lives in arm_kinematics and has two layers:

**Layer 1 — use `CollisionManager` directly.** `CollisionManager::collide()` already has the
right shape. In `update()`, after computing desired joint positions:

```cpp
collision_manager_.update_poses(desired);
if (collision_manager_.collide()) {
  desired = current; // hard-stop
}
```

No wrapper function needed.

**Layer 2 — optional `CollisionLimiter` struct** for controllers that want the enforce-in-place
pattern as a named object:

```cpp
// arm_kinematics/collision/collision_limiter.hpp
struct CollisionLimiter {
  CollisionManager manager;
  // Resets desired to current if desired would cause a collision.
  void enforce(span<const double> current, span<double> desired);
};
```

A `JointLimiterInterface` wrapper can be built on top of this if needed for ros2_control
compatibility, but the primary API does not depend on ros2_control.

**Layer 3 — velocity limiting at boundary** (future). A smarter limiter would reduce velocity
proportionally as the arm approaches a collision boundary rather than hard-stopping. This
requires binary search for the collision frontier and is out of scope for now.

**Ownership:** The controller builds the `CollisionManager` (using the param-reading factory
described in Gap 4) and passes it to the `CollisionLimiter`. The limiter does not own a
`PluginLoader`.

**Impact on state interface acquisition:** If collision checking requires auxiliary joints
(joints outside the controller's command set), the controller must declare state interfaces
for those joints too. The `JointMapBuildError` from `make_collision_manager()`'s internal
`make_tree()` call will surface which joints are missing, with resolution hints.

### Auxiliary joint value sourcing

When collision checking (or other arm_kinematics tools) need joint values that the controller
doesn't command, the values must come from somewhere. In order of preference:

1. **Extra state interfaces** (best). The controller declares additional state interfaces for
   auxiliary joints. Values are read as part of the control loop — timely and consistent.
   `JointMap` maps them alongside commanded joints.

2. **`/joint_states` subscriber** (fallback). A utility subscribes to `/joint_states` on a
   separate thread with RT-safe retrieval. Useful when the auxiliary joints are managed by a
   different controller that doesn't export state interfaces.

3. **Explicit default values** (last resort). Fixed values for joints whose positions don't
   meaningfully affect collision geometry. Must be explicitly opted into — the default should
   be to fail if a required joint value isn't available, not to silently assume zero.

The `JointMapBuilder` interface is designed for extensibility: a custom builder could wrap the
default builder and inject default values for specific joints. This keeps the core builder
honest while allowing controllers to opt in to defaults where appropriate.

---

## What Changes per Controller

### nova_arm_controller

- Build `CollisionManager` via the new param-reading factory (Gap 4)
- Replace `SelfCollisionLimiter`'s MoveIt internals with `CollisionLimiter` (wraps the
  pre-built `CollisionManager`)
- State interface acquisition via Utility B
- No IK needed (joint-level control only)

### nova_twistmapper

- Replace MoveIt kinematics solver with `PluginLoader::make_fk()` + `make_ik()`
- Replace MoveIt FK calls with `Tree::position_fk()`
- Replace MoveIt IK calls with `InverseKinematicsPlugin::get_position_ik()` (and optionally
  `get_velocity_ik()` — configurable via parameter)
- Replace MoveIt self-collision checking with `CollisionManager`
- Implement twist frame swap (Gap 6): detect frame change in subscription callback, rebuild
  FK tree on that thread, hand to RT thread via `RealtimeBox<Tree::SharedPtr>`
- Remove: SRDF fallback generation, compat node, planning scene, allowed collision matrix
  generation, joint handle reordering hack
- State + command interface acquisition via Utilities A and B

### nova_path_planner

- Same MoveIt replacements as twistmapper
- Replace `check_path_for_self_intersection()` with `CollisionManager`-based interpolation
  checking (same algorithm, different collision backend)
- State + command interface acquisition via Utilities A and B

---

## Remaining Gaps in arm_kinematics

### Gap 4: AllowedCollisionMatrix factory

`make_collision_manager` currently takes a caller-supplied `AllowedCollisionMatrix`. There is
no utility to build the ACM from ROS parameters. This needs to be added:

```
Parameters (read by new factory):
  collision.generate_from_zero_pose      bool   default: true
  collision.zero_pose.<joint_name>       double default: 0.0  (per-joint overrides)
  collision.allowed_pairs                string[] default: []  (explicit "link1/link2" pairs)
```

**Implementation:** build the collision plugin with an empty ACM, call
`update_poses(zero_joint_values)` + `collide(pairs, max)`, then mutate the ACM via
`get_allowed_collision_matrix()` to allow the discovered pairs. The explicit
`allowed_pairs` list is added on top. No plugin rebuild needed — the ACM is mutable after
construction.

### Gap 5: Path collision utility

`check_path_for_self_intersection` is duplicated identically between twistmapper and
path_planner. It belongs in arm_kinematics near `CollisionManager`:

```cpp
// arm_kinematics/collision/collision_manager.hpp (or nearby)
// Interpolates N steps between start and end joint positions, checking collision at each.
// Returns true if any intermediate pose (including end) causes a collision.
bool check_path_collision(
    CollisionManager & manager,
    span<const double> start,
    span<const double> end,
    double step_size);
```

This is structurally similar to the collision limiter check (both call
`update_poses()` + `collide()`), so they should live together.

### Gap 6: Twist frame tree swap

The twistmapper resolves the twist's reference frame via MoveIt FK. With arm_kinematics, the
FK tree is built for a specific declared frame. When `frame_id` changes, a new tree is needed.

**Design:**
- `last_frame_id_` and the hook for this already exist in `nova_twistmapper.cpp` (subscription
  callback, with a `// TODO: Do some optimisations and magic tricks here!` comment)
- On frame change (subscription callback thread): call `fk_plugin_->make_tree()` for the new
  frame, wrap the result in a `shared_ptr`, push to a `RealtimeBox<Tree::SharedPtr>`
- RT thread: pull from `RealtimeBox::get()` each tick — wait-free, never blocks on construction
- Old tree destroyed by the subscription thread when `RealtimeBox::set()` returns the
  displaced `shared_ptr` to the caller; RT thread never runs a destructor
- RT thread only ever computes for the one active frame — no multi-frame waste
