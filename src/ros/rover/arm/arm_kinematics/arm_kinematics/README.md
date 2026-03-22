# arm_kinematics

`arm_kinematics` provides reusable runtime helpers for rover arm controllers built around `ros2_control`.
Its core responsibilities are:

- joint-space remapping and transmission propagation through `JointMap`
- forward kinematics
- inverse kinematics
- self-collision checking

This package is built around the idea that these are not separate systems.
They are different runtime views over one shared robot model.

While these data structures are applied in `ros2_control` controllers, algorithms should not depend on the package, and 
be widely applicable. 

## The Right Mental Model

The URDF is the source description of the robot, but it is not the main runtime data structure.
The real runtime work happens in preprocessed arrays and compact helper objects built from that URDF once, then reused in the control loop.

Data structures often go through 3 phases:

1. **Analysis** - Using the URDF, KinematicsParams, and plugin implementations to create reusable analysis structures to 
                  build efficient compute structures from.
2. **Building/Planning** - Creating lightweight compute structures from the analysis structures.
3. **Compute** - Minimal, bulk-math data structures for use in the hot real-time loop.

Shared objects include:

- `RobotModel`: owns the robot description and lazily derives shared URDF-backed structures
- `KinematicsParams`: defines parameters for configuring kinematics in your controller 
- `PluginLoader`: loads FK, IK, and collision plugins against the same shared state. Use this!

If you are changing the package, think in terms of preserving alignment between these runtime views rather than treating FK, IK, collision, and joint mapping as independent features.

No single controller should have to opt into all of these systems, and should be able to pick and choose as needed.

## Joint Maps

`JointMap` is core to the design of the package, but often misunderstood.

It maps joint values in the functional programming sense, not like a hashmap.

- It reorders joint values from the order your controller defines into an order that is efficient for the kinematics algorithms.
- It is also able to apply transmissions and mimic joints. Get this from your FK plugin, as the FK plugin might include additional transmissions.

Maintain this distinction carefully:

- `TransmissionAnalysis` is the semantic owner of transmission structure, including affine transmission relationships derived from mimic joints.
- `TransmissionModel` is only for grouped non-affine transmission compute that may need quantity-specific build behavior.
- Mimics should be normalized during analysis into affine transmission relationships, not modeled through `TransmissionModel`.
- `AffineJointMap` is the fast compiled execution form for reorder and affine transmission cases. It is not where mimic semantics should originate.
- Many affine transmission relationships for one request should usually compile into one `AffineJointMap`, not one runtime stage per relationship.
- In the longer term, mixed requests should be split into affine execution segments separated by genuinely non-affine transmission stages.

## Setup-Time Work Versus Runtime Work

Always keep structural work out of the real-time path.

Setup-time work includes:

- parsing the URDF
- discovering mimic joints and normalizing them into affine transmission relationships
- parsing transmission metadata
- reducing trees to the requested joints and frames
- sorting joints and frames into compute order
- constructing FK trees, `JointMap`s, and collision geometry

Runtime work should stay close to:

- map joint values through a prebuilt `JointMap`
- update preallocated FK outputs
- update collider poses
- run collision queries
- run IK solver math

Avoid introducing heap allocation or repeated name-based lookups into code that will be used in a control loop.

## Main Entry Points

- [`include/arm_kinematics/common/robot_model.hpp`](include/arm_kinematics/common/robot_model.hpp): shared robot description and cached derived structures
- [`include/arm_kinematics/joint_map/joint_map.hpp`](include/arm_kinematics/joint_map/joint_map.hpp): runtime joint-space mapping abstraction
- [`include/arm_kinematics/forward/forward_kinematics_plugin.hpp`](include/arm_kinematics/forward/forward_kinematics_plugin.hpp): FK plugin interface
- [`include/arm_kinematics/collision/discrete_collision_plugin.hpp`](include/arm_kinematics/collision/discrete_collision_plugin.hpp): collision plugin interface
- [`include/arm_kinematics/inverse/inverse_kinematics_plugin.hpp`](include/arm_kinematics/inverse/inverse_kinematics_plugin.hpp): IK plugin interface
- [`include/arm_kinematics/plugin_loader.hpp`](include/arm_kinematics/plugin_loader.hpp): shared plugin initialization helper

The default concrete implementations are:

- `EigenForwardKinematicsPlugin`
- `FclCollisionPlugin`
- `BanksiaIKPlugin`

## Recommended Reading Order

For maintainers, this is the shortest useful path through the codebase:

1. [`docs/implementation_guide.md`](docs/implementation_guide.md)
2. [`docs/joint_map_guide.md`](docs/joint_map_guide.md)
3. `RobotModel`, `JointMap`, and `PluginLoader`
4. `ForwardKinematicsPlugin` and `EigenForwardKinematicsPlugin`
5. `CollisionManager` and `FclCollisionPlugin`
6. the tests in [`test/`](test)

[`kinematics.hpp`](include/arm_kinematics/kinematics.hpp) provides an example of how you can initialize various data 
structures. You do not need to use this type, and I encourage you to build your own Kinematics class specific to your 
controller, using [`kinematics.hpp`](include/arm_kinematics/kinematics.hpp) as a guide.

## Working Style In This Package

Prefer to use Bailey's existing style for public APIs and comments:

- use short, direct Doxygen comments on public interfaces
- explain the contract and invariants, not obvious syntax
- keep runtime-facing code simple and explicit
- prefer preallocation and compact data transforms over convenience abstractions in hot paths
- always prefer composition
- implement features using explicit dependency injection, then build helper wrappers around that injection to make 
  passing dependencies around easier. Don't assume any singletons.
