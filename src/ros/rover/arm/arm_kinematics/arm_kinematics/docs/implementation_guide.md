# arm_kinematics Implementation Guide

This package is trying to solve a different problem from MoveIt-style planning stacks.
It is focused on giving a `ros2_control`-friendly controller a compact, reusable set of runtime helpers for:

- forward kinematics
- inverse kinematics
- self-collision checking

The core design choice is that the package does not treat FK, IK, and collision as three separate worlds.
Instead, they all derive their view of the robot from one shared `RobotModel`, then build specialized runtime data structures from that shared source.

This guide is for developers who already understand the high-level robotics concepts, but want to understand what this library expects from an implementation and how the data structures line up.

## The Mental Model

At a high level, the data flow is:

1. A controller owns one `RobotModel`.
2. Plugins read shared configuration from `KinematicsParams`.
3. FK builds a reduced compute tree for a chosen root frame and a chosen set of requested output frames.
4. Joint inputs from the controller are remapped into the ordering required by that compute tree through a `JointMap`.
5. Collision builds on top of FK by treating colliders as just another set of frames whose poses need to be updated.
6. IK plugins reuse the same `RobotModel` and `KinematicsParams`, but solve for joint states instead of consuming them.

The most important consequence is this:

The URDF is authoritative, but it is not the runtime representation.

The runtime representation is a set of preprocessed arrays that are ordered for fast repeated updates inside a control loop.

## The Main Shared Objects

### `RobotModel`

[`RobotModel`](../include/arm_kinematics/common/robot_model.hpp) is the shared root object.
It owns the robot description string and lazily derives reusable views from it:

- `urdf::Model`
- `JointMapBuilder`
- `AnalysisTree`

The intent is that your controller creates one `RobotModel` and keeps it alive for as long as any plugin instance or derived tree depends on it.
That matters because the plugins are designed to share derived URDF data rather than re-parsing independently.

### `KinematicsParams`

[`KinematicsParams`](../include/arm_kinematics/common/kinematics_params.hpp) holds the common runtime configuration:

- `base_link_name`
- `ee_link_name`
- `joint_names`

These are not a replacement for the URDF.
They are the controller's declared operating subset and defaults.

In practice:

- the URDF says what the robot is
- `KinematicsParams` says what part of it this controller cares about by default

### `PluginLoader`

[`PluginLoader`](../include/arm_kinematics/plugin_loader.hpp) is a convenience layer.
It owns a `RobotModel`, lazy-loads common parameters, instantiates pluginlib classes, and initializes them consistently.

You do not have to use it, but the library is clearly shaped around the idea that FK, IK, and collision plugins should all be initialized against the same robot description and parameter set.

## Why The Library Reorders Things

If you have mostly worked at the controller level, your natural assumption is often:

"I have joint names in my command/state interfaces, and I want poses out in the same order I asked for them."

That is convenient for API ergonomics, but it is not the best layout for repeated kinematics updates.

This library deliberately separates:

- caller-facing ordering
- compute-friendly ordering

The compute-friendly ordering exists so the runtime tree can be updated with simple forward passes over contiguous arrays.
Parents must appear before children.
Frames are grouped so varying outputs are handled first and root-relative constants are pushed to the end.

That is why `ForwardKinematicsPlugin::make_tree()` returns both:

- a tree object
- an `Order<>` describing how requested frames were rearranged

If you are implementing with this library, do not treat reordering as an edge case.
It is part of the contract.

## FK: From URDF To Runtime Arrays

The FK implementation is easiest to understand as a pipeline.

### 1. `AnalysisTree` converts the URDF into a structural model

[`AnalysisTree`](../include/arm_kinematics/forward/utilities/analysis_tree.hpp) is not yet the fast runtime object.
It is an intermediate representation that makes the robot easier to reshape.

Its key ideas are:

- non-fixed joints become `Joint` nodes
- links and fixed-link offsets become `Frame` entries
- there is always a dummy root joint at index `0`
- the structure stays topologically sortable

The library treats fixed joints differently from actuated joints:

- actuated joints belong in the joint tree
- fixed transforms are represented as frame offsets relative to the nearest actuated ancestor

This split is what lets the runtime update path avoid revisiting fixed geometry unnecessarily.

### 2. `AnalysisTree` is reduced to the requested root and frames

The Eigen FK plugin does not run FK over the entire URDF every time.
Instead, it constructs a subtree:

- rooted at the caller's chosen `base_link_name`
- containing only the joints and frames needed for the requested outputs

This is the role of the `AnalysisTree(other, root_name, definitions)` constructor.

That constructor does a lot of the conceptual heavy lifting in this package:

- it finds the requested root frame in the original URDF-derived tree
- it reverses the path from that root back toward the URDF root when needed
- it gathers only the joints required to reach the requested output frames
- it rewrites frame origins so all outputs are expressed in the new chosen root frame

This is why "implementing FK" here is not just about applying joint transforms.
A large part of the implementation problem is choosing and reshaping the correct subset of the robot.

### 3. Joints are sorted into compute order

`AnalysisTree::sort_joints()` enforces the ordering needed for a forward sweep.
The resulting order guarantees that when a pose is computed, its parent pose has already been computed.

After sorting, `AnalysisTree::make_compute_joint_tree()` emits a compact [`ComputeJointTree`](../include/arm_kinematics/forward/utilities/compute_joint_tree.hpp) with parallel arrays:

- joint types
- joint axes
- origin transforms
- parent indices for non-root-relative joints
- count of root-relative joints

This is the first truly data-oriented runtime structure in the FK stack.

### 4. Frames are sorted into output order for the compute tree

`AnalysisTree::sort_frames()` then reorders requested frames so that:

- frames driven by joint poses come first
- root-relative constant frames come last

That allows [`ComputeFrameTree`](../include/arm_kinematics/forward/utilities/compute_frame_tree.hpp) to update dynamic outputs from joint poses, then append constant transforms cheaply.

`ComputeFrameTree` stores:

- the `ComputeJointTree`
- for each varying frame, which joint pose is its parent
- the final fixed offset from that parent joint pose to the requested frame

At runtime, updating a frame tree is then:

1. update all joint poses
2. multiply each requested frame's parent joint pose by its stored offset
3. copy precomputed constants for root-relative frames

### 5. `JointMap` bridges controller ordering to compute ordering

The compute tree's joint order is not assumed to match the controller's joint order.

[`JointMap`](../include/arm_kinematics/joint_map/joint_map.hpp) exists to transform caller-provided joint states into the order required by the compute tree.
Its runtime representation is intentionally simple:

- `sources`
- `multipliers`
- `offsets`

For each output joint value:

`output[i] = input[sources[i]] * multipliers[i] + offsets[i]`

This is how the library handles:

- caller ordering differences
- mimic joints
- simple affine remappings

The key implementation point is that the FK tree itself does not care where a joint value came from.
It only wants a dense vector in the order its compute arrays expect.
`JointMap` is the adapter that makes that possible.

### 6. The concrete default FK plugin

[`DefaultForwardKinematicsPlugin`](../include/arm_kinematics/plugins/forward/default_forward_kinematics_plugin.hpp) packages the above pieces into the default FK implementation.

Its `TreeImpl` holds:

- `ComputeFrameTree`
- `JointMap`
- a reusable buffer for mapped joint states

Runtime FK becomes:

1. map `std::vector<double>` joint inputs into the tree's float-ordered joint buffer
2. update the compute frame tree into the caller's preallocated output pose vector

This is the package's main pattern:

expensive structure building happens outside the real-time loop, while repeated updates become array operations over preallocated storage.

## Collision: FK Reused For Collider Poses

Collision is not modeled as a separate kinematics system.
It is modeled as:

- a set of geometries
- an allowed collision filter
- an FK tree that tells you where those geometries are

### `ColliderDefinitions`

[`ColliderDefinitions`](../include/arm_kinematics/collision/collider_definitions.hpp) extracts three aligned collections from the URDF:

- `colliders`: references to `urdf::Collision`
- `frames`: `FrameDefinitions` describing where each collider sits relative to its parent link
- `acm`: an `AllowedCollisionMatrix`

The important word is aligned.
Index `i` means the same collider across all three collections.

That alignment must be preserved whenever ordering changes.

### Why collision depends on FK ordering

To run collision, the plugin needs collider poses in the same order as the collision geometry list it initialized with.

But FK may reorder frames for compute efficiency.
So the collision setup path does this:

1. build `ColliderDefinitions`
2. ask FK to build a tree for collider frames
3. receive the frame reorder `Order<>`
4. reorder collider geometry references to match the FK tree's output order
5. initialize the collision plugin with that reordered geometry list

This is one of the most important relationships in the codebase.
The collision plugin does not discover pose ordering on its own.
It trusts FK's frame order and makes its geometry ordering match that order.

### `AllowedCollisionMatrix`

[`AllowedCollisionMatrix`](../include/arm_kinematics/collision/allowed_collision_matrix.hpp) is a packed triangular bitset over collider pairs.

Conceptually:

- `true` means "ignore this pair"
- `false` means "this pair should be checked"

The matrix is seeded from URDF collider grouping rules in `ColliderDefinitions`:

- colliders on the same link are allowed to intersect

The collision plugin then uses this filter before narrow-phase checks.

If you are implementing or extending collision logic, keep two invariants in mind:

1. ACM indices refer to collider indices, not link indices.
2. Any reordering of collider collections must be reflected consistently wherever those indices are used.

### `DiscreteCollisionPlugin`

[`DiscreteCollisionPlugin`](../include/arm_kinematics/collision/discrete_collision_plugin.hpp) is intentionally narrow in scope.
It does not know joint states.
It does not know FK.
It only knows:

- what geometries exist
- what poses they currently have
- which pairs are allowed

That means the collision plugin boundary is clean:

- kinematics owns pose generation
- collision owns geometric intersection testing

### `CollisionManager`

[`CollisionManager`](../include/arm_kinematics/collision/collision_manager.hpp) is the bridge object that ties those two concerns together.

It owns:

- an FK tree for colliders
- a collision plugin
- a preallocated pose buffer

Its runtime flow is:

1. `update_poses(joint_states)` computes collider poses from FK
2. those poses are pushed into the collision plugin
3. `collide()` asks the collision plugin whether the current pose set self-intersects

This is a good example of the library's decomposition style:
the manager is small because the real work was already split into aligned data structures during initialization.

## IK: Shared Inputs, Different Runtime Contract

[`InverseKinematicsPlugin`](../include/arm_kinematics/inverse/inverse_kinematics_plugin.hpp) shares the same base initialization model as FK:

- it gets node interfaces
- it gets the shared `RobotModel`
- it gets shared `KinematicsParams`

But its runtime contract is different.
Instead of building a generic tree object, the plugin implementation is responsible for solving:

- position IK through `get_position_ik()`
- optionally velocity IK through the default finite-difference-based `get_velocity_ik()`

The key idea is that IK is expected to be robot-specific.
The package does not try to provide a generic numerical solver.

For an IK implementation, the shared data still matters:

- `KinematicsParams::joint_names` defines the expected joint ordering of the solution vectors
- `base_link_name` and `ee_link_name` define the default reference frames
- the shared `RobotModel` gives access to URDF-derived structural information if the solver needs it

So the package treats IK as part of the same ecosystem, but not as part of the same generic compute-tree machinery.

## What A Plugin Author Actually Has To Implement

### Forward kinematics plugin authors

A FK plugin author does not need to redefine the whole package architecture.
The expected responsibilities are:

- accept a requested joint set, root frame, and output frame definitions
- build an efficient runtime tree for repeated `position_fk()` calls
- return any output frame reordering that your compute layout requires
- use `JointMapBuilder` or an equivalent mechanism if your internal joint order differs from the caller's

What the package already gives you:

- common initialization via `KinematicsBase`
- a shared `RobotModel`
- parameter loading through `KinematicsParams`
- a default URDF-derived `JointMapBuilder`
- the existing `AnalysisTree`/`Compute*Tree` model if you want to build on it

### Collision plugin authors

A collision plugin author is responsible for:

- converting URDF collision geometries into the collision library's geometry types
- storing and updating poses for each collider
- implementing broad-phase and narrow-phase queries
- respecting the `AllowedCollisionMatrix`

The plugin is not responsible for:

- interpreting joint states
- resolving frame transforms from the robot model
- deciding collider ordering

Those are upstream responsibilities.

### IK plugin authors

An IK plugin author is responsible for:

- defining the robot-specific solution logic
- consuming and returning joint vectors in the agreed order
- selecting the valid solution closest to the provided seed state when multiple solutions exist

The package does not currently impose a generic analytic IK representation beyond that interface.

## The Important Alignment Rules

If you only remember one section from this guide, make it this one.

This package relies heavily on parallel vectors and shared indexing conventions.
Most correctness bugs in implementations around this design will come from broken alignment rather than bad math.

The main alignment rules are:

1. `KinematicsParams::joint_names` defines the caller-facing default joint order.
2. FK compute trees may use a different internal joint order.
3. `JointMap` is what translates between those two orders.
4. Requested frame order may be changed by FK tree construction.
5. The returned `Order<>` tells you how FK reordered those frames.
6. Collision geometry order must match FK collider-pose output order.
7. `AllowedCollisionMatrix` indices refer to that final collider order.

If two structures are supposed to describe the same entities, ask:

"Are they aligned by name, or by shared index?"

In this package, the answer is often "by shared index after reordering."

## Real-Time Versus Setup-Time Responsibilities

The codebase is designed around a strong separation between setup work and runtime work.

Setup-time work includes:

- parsing the URDF
- discovering mimic joints
- parsing ros2_control transmission XML
- reducing the robot to a requested subtree
- sorting joints and frames
- constructing compute trees
- constructing collision geometry objects

Runtime work is expected to be:

- map joint inputs
- update tree poses
- update collider poses
- run collision queries
- run solver math for IK

When extending the library, a good rule is:

If it depends only on robot structure or requested outputs, try to do it once during initialization.
If it depends on the current joint state, keep it in the runtime path and make it operate on preallocated arrays.

## Current Implementation Notes

A few details in the current code are important for developers reading or extending it:

- The default FK implementation is the Eigen-based compute-tree plugin.
- The default collision implementation is the FCL plugin.
- IK has an interface but no default generic implementation.
- `JointMapBuilder` already parses transmission XML, but the current `JointMap::build()` path only applies mimic-joint-style affine remapping. Transmission support is not yet wired through in the runtime mapping logic.
- Collision meshes are currently treated conservatively: unsupported mesh geometry is skipped, and cylinders are converted to capsules in the FCL plugin's geometry conversion layer.

That means the library architecture is ahead of some implementation details.
When documenting or extending it, distinguish clearly between:

- what the interfaces are designed to support
- what the current concrete implementation actually supports today

## A Good Way To Think About Implementing With This Package

If you are used to writing `ros2_control` controllers, the most useful shift is this:

Do not think of FK, collision, and IK as black-box library calls.
Think of them as cooperating views over one shared robot description.

The implementation job is mostly to preserve these relationships:

- one authoritative robot description
- one agreed joint ordering at the controller boundary
- explicit remapping into compute ordering
- explicit frame reordering when building runtime trees
- explicit index alignment between pose outputs, colliders, and ACM entries

Once those relationships are correct, the actual runtime math becomes comparatively straightforward.

That is the core pattern this package is building around.
