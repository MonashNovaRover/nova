# arm_kinematics Overview

`arm_kinematics` is a shared runtime support library for rover arm controllers built on
`ros2_control`.

It groups together several pieces of arm runtime infrastructure that need to agree on the same
robot model:

- forward kinematics
- inverse kinematics
- self-collision checking
- joint-space remapping and transmission propagation
- ROS 2 control interface naming and lookup helpers

The package is designed around one idea: these are different runtime views over the same robot,
not separate systems with separate configuration.

That shared-model design avoids a common failure mode in controller code: FK, IK, collision, and
joint remapping silently drifting apart because each subsystem parsed or interpreted the robot
description differently.

## Mental Model

The URDF is the source description of the robot, but it is not the main runtime data structure.
Most useful work in this package happens in structures that are built once from the URDF, then
reused in controllers.

The package naturally falls into three phases:

1. Analysis
   Parse the URDF, register joints and interfaces, import transmissions, normalize mimic-style
   affine relationships, and derive shared structural data.
2. Build
   Turn those structural descriptions into compact helpers such as FK trees, collision objects,
   and `JointMap`s.
3. Runtime
   Reuse the prebuilt helpers inside the control loop with fixed-size vectors and preallocated
   outputs.

As a rule, expensive structural work belongs in controller initialization, not in `update()`.

## Why The Layers Are Split

The package keeps analysis, build, and runtime as separate layers for two reasons:

- correctness
  Structural decisions such as joint ordering, transmission reachability, and frame reduction are
  made once, in one place, instead of being re-derived differently by each algorithm
- real-time safety
  URDF parsing, plugin loading, graph analysis, and name-based lookup are all acceptable during
  setup, but are bad fits for a control loop

If a structure looks more complicated than a direct vector transform, it usually exists to push
that complexity out of the hot path.

## Main Structures

### Shared setup structures

- `RobotModel`
  Owns the robot description and lazily derives shared URDF-backed data such as the parsed URDF,
  default `TransmissionAnalysis`, and analysis tree. Use it when multiple plugins or helpers need
  to agree on the same robot description. Hold it for the lifetime of the controller setup that
  depends on it.
- `KinematicsParams`
  Reads the common controller-facing kinematics parameters such as `base_link_name`,
  `ee_link_name`, and `joint_names`. Use it to keep controller/plugin configuration aligned
  instead of passing these names around ad hoc.
- `PluginLoader`
  The usual entry point for controller code. It owns the shared `RobotModel`, lazily reads
  `KinematicsParams`, and loads FK, IK, and collision plugins against the same shared state. Use
  it when you want one place to build the controller's kinematics stack consistently.

### Runtime-facing kinematics and collision structures

- `ForwardKinematicsPlugin`
  Abstract FK interface. Controller code usually creates one through `PluginLoader`. Use it when
  you need to build reusable trees for one or more tracked frames.
- `ForwardKinematicsPlugin::Tree`
  A prebuilt FK object that maps ordered joint states to ordered frame poses. This is the object
  you actually want in runtime code.
- `InverseKinematicsPlugin`
  Abstract IK interface. Used to solve either target poses or target twists into joint commands.
  Use it when the controller reasons in task space but needs joint-space commands.
- `CollisionManager`
  Convenience wrapper that ties together an FK tree for colliders and a collision plugin.
  Controllers update collider poses with joint states, then query for collisions. Use it when you
  want one runtime object that owns the "update poses, then query collision" workflow.

### Joint map and transmission structures

- `NamedStateInterfaceDefinition`
  Joint-name-based description used at API boundaries and in controller code. Use it when the
  caller still thinks in terms of controller joint names.
- `StateInterfaceDefinition`
  Joint-id-based description used after names have been resolved into analysis-local ids. Use it
  when you are already inside analysis/build logic and want stable ids instead of names.
- `TransmissionAnalysis`
  Build-time graph of joints, interfaces, affine relationships, and transmission instances. Use
  it to answer structural questions once, then build cheaper runtime objects from the result.
- `JointMapBuilder`
  Build-time planner that answers: "given these inputs, how do I produce these outputs?" Use it
  during initialization when you need a runtime mapping object with transmission policy baked in.
- `JointMap`
  Runtime mapping object produced by a builder. This is the object you reuse in hot paths instead
  of re-running reachability or transmission analysis.

### ROS 2 control helpers

- `arm_kinematics::ros2_control::state_interface_names(...)`
- `arm_kinematics::ros2_control::command_interface_names(...)`
- `arm_kinematics::ros2_control::find_state_interface_refs(...)`
- `arm_kinematics::ros2_control::find_command_interface_refs(...)`

These helpers keep interface declaration order and acquired handle order aligned with the vectors
you use later in controller code.

## Setup-Time Vs Runtime Use

These are usually setup-time only:

- `RobotModel`
- `KinematicsParams`
- `PluginLoader`
- `TransmissionAnalysis`
- `JointMapBuilder`
- `ForwardKinematicsPlugin::make_tree(...)`
- `make_collision_manager(...)`

These are the reusable runtime helpers built from that setup:

- `ForwardKinematicsPlugin::Tree`
- `JointMap`
- `CollisionManager`
- FK / IK plugin instances once initialized

## Common Footguns

- Rebuilding trees, maps, or collision managers inside `update()`
  This moves allocation, graph analysis, and plugin work into the hot path.
- Mixing joint orders across controller vectors
  Most APIs here are order-sensitive. If setup used `params_.joint_names`, runtime vectors should
  usually follow that exact order too.
- Treating `NamedStateInterfaceDefinition` and `StateInterfaceDefinition` as interchangeable
  One is a name-based boundary type, the other is an id-based build-time type.
- Assuming requested frame order always matches returned FK pose order
  `make_tree(...)` may reorder frames; use `frame_order` when that matters.

## Typical Controller Flow

The common controller flow in `arm_controllers` is:

1. Read `robot_description`.
2. Construct `PluginLoader`.
3. Create FK and optionally IK plugins.
4. Build collision support if the controller needs it.
5. Build one or more FK trees for the frames the controller cares about.
6. Acquire ordered ROS 2 state and command interface refs.
7. Reuse the prebuilt trees, maps, and collision manager in `update()`.

For concrete controller patterns, see:

- [controller_usage.md](controller_usage.md)
- [joint_map_and_transmissions.md](joint_map_and_transmissions.md)
