# Dynamics Feasibility

This report investigates whether inverse dynamics would fit `arm_kinematics` as it exists today,
with emphasis on the same analysis/build/runtime layering already used for forward kinematics,
collision, and joint-space remapping.

The report is based on:

- the existing package documentation:
  `overview.md`, `joint_map_and_transmissions.md`, `controller_usage.md`, and `package_uml.md`
- the current implementation of:
  `RobotModel`, `AnalysisTree`, `ComputeJointTree`, `ComputeFrameTree`,
  `TransmissionAnalysis`, `TransmissionReachability`, `JointMapBlueprint`,
  and `materialize_joint_map`
- external primary references:
  Featherstone's spatial-vector material, Pinocchio documentation, RBDL documentation,
  Orocos KDL documentation, and Drake multibody documentation

## Short Answer

Yes, it is feasible to add inverse dynamics to this library, and the current codebase already has
some of the right structural habits for it:

- immutable analyzed structure derived from URDF
- explicit build-time vs runtime separation
- contiguous topological arrays instead of runtime graph traversal
- ordered-vector runtime APIs

However, the package does **not** currently contain the main prerequisites that make inverse
dynamics possible:

- inertial data structures
- rigid-body inertia representation
- first-order and second-order rigid-body kinematic recursions
- a representation for generalized forces and external wrenches
- a clear distinction between mechanism-space generalized forces and actuator-space efforts

So the answer is:

- feasible as a staged extension
- not a small feature
- best approached as a new set of low-level analysis/build/runtime structures, not as a plugin

## What The Existing Codebase Already Gives You

The current package already has several properties that are unusually helpful for a dynamics
implementation.

### 1. Shared analyzed robot structure

`RobotModel` already acts as a shared, lazily-built source of robot structure. Today it exposes:

- parsed URDF
- default `TransmissionAnalysis`
- `AnalysisTree`

That is the same ownership shape a dynamics stack wants: parse once, analyze once, then build
specialized runtime helpers from stable analyzed state.

### 2. A compact topological tree representation

`AnalysisTree` is already close to the sort of structural representation used in rigid-body
dynamics:

- joints are topologically ordered
- fixed joints are reduced into frame offsets
- each actuated joint stores parent, origin, axis, type, children, and attached frames
- subtrees can be re-rooted for a requested base link

That is a strong starting point. Featherstone's model structure also depends on a parent array and
per-joint transforms arranged in topological order, and the current FK tree builder already
enforces that kind of layout.[^featherstone-model]

There is also a useful conceptual match with Pinocchio's treatment of fixed joints: fixed joints
are typically better treated as operational frames than as dynamic DoFs.[^pin-fixed]

### 3. Runtime objects are already data-oriented

`ComputeJointTree` and `ComputeFrameTree` are not generic object graphs. They are compact runtime
executors over parallel arrays:

- joint types
- joint axes
- zero-position transforms
- parent indices
- output pose buffers

That is exactly the direction a good dynamics runtime should follow. A future dynamics runtime
object should look much more like `ComputeJointTree` than like a solver hierarchy with heavy
virtual dispatch in the loop.

### 4. The package already separates semantic mapping from runtime execution

The joint-map/transmission subsystem is especially relevant.

The current design is:

- `TransmissionAnalysis`: semantic relationships and stable ids
- `TransmissionReachability`: build-time derivability analysis
- `JointMapBlueprint`: ordered execution plan
- `JointMap`: runtime executor

That decomposition matters for dynamics because it suggests the right architecture:

- analyze dynamic structure once
- compile small runtime helpers
- keep controller-loop execution to ordered spans and preallocated scratch

This is a better fit than trying to hide everything behind a single "inverse dynamics solver"
object.

## What Is Missing Today

The current package is still fundamentally kinematics-oriented.

### No inertial model

The code currently stores:

- geometry
- joint axes and transforms
- transmission semantics

It does **not** store the inertial parameters needed for rigid-body dynamics:

- body mass
- center of mass
- rotational inertia
- a rigid-body inertia representation in joint/body coordinates

Without that, inverse dynamics cannot begin.

### The 6D motion/force layer is now only partial

The package now has dedicated `Twist` and `Wrench` types, with:

- repo-consistent ordering:
  `Twist = [linear; angular]`, `Wrench = [force; moment]`
- named accessors and zero-overhead Eigen-compatible storage
- direct operations with `Eigen::Isometry3d`
- named motion/force cross-product helpers and power pairing

That removes one of the original blockers: there is now a usable strongly-typed 6D motion/force
foundation for dynamics work.

What is still missing is the rest of the dynamics-facing layer:

- spatial inertia matrices
- inertial re-expression helpers
- first-order and second-order recursion state built around those types

In other words, a separate `SpatialTransform` type is not required here, but `Twist`/`Wrench`
alone are not yet enough to implement inverse dynamics end-to-end. Every modern library in this
space still needs the inertia and recursion pieces built on top of the same 6D algebra.[^featherstone-software]
[^rbdl-joints]

### No velocity/acceleration recursion

`ComputeJointTree` computes poses only.

Inverse dynamics needs, at minimum:

- joint motion subspaces
- body spatial velocities
- bias accelerations
- body spatial accelerations
- accumulated body forces

That means the current FK runtime layer is necessary background, but not close to sufficient.

### No generalized-force model

This package has rich treatment of position/velocity/acceleration interface propagation, but it is
careful not to assume that affine kinematic relationships automatically imply effort propagation.
That caution is correct.

For dynamics, this means there are really two different problems:

1. mechanism-space inverse dynamics:
   compute generalized forces required by the rigid-body model
2. actuation-space mapping:
   map those generalized forces to actuator efforts, motor torques, or transmission loads

Those problems should stay separate here as well.

### Current joint representation is still 1-DoF-centric

The current `JointType` enum only covers:

- revolute
- continuous
- prismatic

That is enough for a first fixed-base implementation, but not enough for a general dynamics
library if you really mean "both fixed-base and floating/mobile-base" and want the structures to
stay generalizable.

## Feasible Direction: Build Dynamics The Same Way FK Was Built

The strongest path is to follow the same package philosophy already visible in FK and joint maps:

1. analyze once from URDF and related semantic inputs
2. build immutable compact model structures
3. run runtime algorithms against ordered vectors and caller-provided scratch

The main difference is that dynamics needs more layers of prerequisite math before the top-level
algorithms become available.

The rest of this report is grouped by those prerequisite and algorithmic layers.

## Algorithm 1: Build A Dynamics Model

This is the first unit problem to solve.

The package needs a build-time structure that plays the same role for dynamics that
`AnalysisTree` plays for FK.

### What it should contain

At minimum, a dynamics model should hold:

- a stable joint/body ordering
- parent indices in topological order
- zero-configuration parent-to-joint transforms
- joint motion subspaces
- per-body rigid-body inertias
- optional named frame attachments for body-relative queries
- root metadata
- gravity convention

In other words, something structurally close to Featherstone's model fields:

- `parent`
- `Xtree`
- `I`[^featherstone-model]

### How it should relate to existing structures

It should be built from the same robot description and probably from the same analyzed topology as
`AnalysisTree`, not from a second independent tree parser.

The likely relationship is:

- `AnalysisTree` stays the geometric/topological source
- a new `DynamicsModel` or `DynamicsAnalysisTree` enriches that topology with inertial and
  motion-subspace data

That means the package keeps one understanding of:

- parenthood
- base-link rebasing
- fixed-joint reduction
- frame naming

### Fixed joints should remain reduced

This codebase already treats fixed joints as frames, not as dynamic DoFs. That is a good design
and should carry forward into dynamics.

There are two reasonable implementation choices:

- keep fixed descendants as frame attachments and fold their inertias into the nearest moving body
- keep an analyzed record of fixed bodies for query/debug purposes, but still merge their inertias
  out of the runtime dynamics recursion

Both Pinocchio and RBDL document this same general idea in different forms.[^pin-fixed]
[^rbdl-joints]

### Suggested API surface

Possible low-level types:

```cpp
struct RigidBodyInertiad;

struct DynamicsModel {
  std::vector<size_t> parents;
  std::vector<JointModel> joints;
  std::vector<Eigen::Isometry3d> Xtree;
  std::vector<RigidBodyInertiad> inertias;
  std::vector<size_t> frame_parent_bodies;
  std::vector<Eigen::Isometry3d> frame_offsets;
  size_t root_dof_count = 0;
};
```

Possible build entry points:

```cpp
DynamicsModel build_dynamics_model(const RobotModel & robot_model);
DynamicsModel build_dynamics_model(
  const RobotModel & robot_model,
  const std::string & base_link_name,
  const FrameDefinitions & frames);
```

The important part is not the exact names. The important part is that there should be one compact,
immutable model object that every later dynamics algorithm reads from.

## Algorithm 2: Compute First-Order And Second-Order Kinematic State

Before inverse dynamics itself, the library needs the runtime equivalent of a dynamics-aware
`ComputeJointTree`.

### Unit problems to solve

Given:

- `q`
- `v`
- optionally `a`

compute:

- per-joint transform updates
- per-body spatial velocity
- per-body bias acceleration terms
- per-body spatial acceleration

This runtime state should be explicitly separated from the immutable model, in the same spirit as
Pinocchio's model/data split.[^pin-model-data]

### Why this is a distinct step

This is where the library stops being "FK plus some force math" and becomes an actual rigid-body
dynamics foundation.

If this layer is built cleanly, then several later algorithms become straightforward extensions:

- inverse dynamics
- gravity compensation
- bias-force extraction
- mass-matrix construction
- Jacobian evaluation
- forward dynamics

### Suggested data object

```cpp
struct DynamicsData {
  std::vector<Eigen::Isometry3d> Xup;
  std::vector<Twistd> v;
  std::vector<Twistd> c;
  std::vector<Twistd> a;
  std::vector<Wrenchd> f;
  std::vector<RigidBodyInertiad> Ic;
};
```

The important property is that this object should be:

- reusable
- preallocated
- runtime-only
- independent of naming and URDF parsing

That matches the package's existing hot-path philosophy.

## Algorithm 3: Recursive Newton-Euler Inverse Dynamics

Once the model and runtime state exist, inverse dynamics itself is straightforward in scope:

- forward pass:
  transforms, velocities, accelerations
- backward pass:
  propagate forces and project them onto joint motion subspaces

This is the standard Recursive Newton-Euler Algorithm (RNEA), which Pinocchio, RBDL, KDL, and
Featherstone all expose in one form or another.[^pin-dynamics] [^rbdl-dynamics]
[^kdl-rne] [^featherstone-software-v2]

### Why this fits the current codebase

RNEA is especially compatible with the existing package style because it only needs:

- topological order
- parent indices
- compact per-joint/per-body arrays
- no runtime tree search

That is already how `ComputeJointTree` operates.

### Suggested first public computations

The first dynamics-facing user operations should probably be:

```cpp
void inverse_dynamics(
  const DynamicsModel & model,
  DynamicsData & data,
  span<const double> q,
  span<const double> v,
  span<const double> a,
  span<const Wrenchd> external_forces,
  span<double> tau_out);

void gravity_forces(
  const DynamicsModel & model,
  DynamicsData & data,
  span<const double> q,
  span<double> tau_out);

void bias_forces(
  const DynamicsModel & model,
  DynamicsData & data,
  span<const double> q,
  span<const double> v,
  span<const Wrenchd> external_forces,
  span<double> tau_out);
```

These are a good first set because:

- `gravity_forces(q)` is a useful controller primitive
- `bias_forces(q, v)` is useful independently of full inverse dynamics
- all three can share one recursion core

### Important semantic boundary: generalized forces first

The output of RNEA should be generalized forces in the model's internal ordering, not actuator
torques in some controller-specific ordering.

That keeps the physics layer honest.

If actuator effort mapping is needed later, it should be a separate stage that explicitly depends
on actuation/transmission semantics, just as current FK depends on `JointMap` instead of baking
remapping into the tree math.

## Algorithm 4: Composite Rigid Body Algorithm And HandC-Style Outputs

If inverse dynamics is being added seriously, mass-matrix support should be planned early even if
it lands slightly later.

The reason is that many useful derived computations want:

- `M(q)`
- `C(q, v)v + g(q)` or similar bias-force terms

Pinocchio and RBDL both expose this layer explicitly through CRBA and related helpers.[^pin-dynamics]
[^rbdl-dynamics]

### Why this matters to this package

The user request is not only "can we compute torques?" but "what unit problems compose the larger
problems?"

`M(q)` and bias terms are one of those unit problems.

They unlock:

- inverse dynamics checks
- operational-space controllers
- constrained dynamics formulations
- forward dynamics without re-deriving structure elsewhere

### Suggested APIs

```cpp
void mass_matrix(
  const DynamicsModel & model,
  DynamicsData & data,
  span<const double> q,
  Eigen::Ref<Eigen::MatrixXd> M_out);

void hand_c(
  const DynamicsModel & model,
  DynamicsData & data,
  span<const double> q,
  span<const double> v,
  Eigen::Ref<Eigen::MatrixXd> M_out,
  span<double> bias_out);
```

Featherstone's `HandC` interface is especially relevant here because it packages exactly the split
many controllers want: mass matrix plus bias vector from one analyzed model.[^featherstone-software-v2]

## Algorithm 5: External Wrenches, Jacobians, And Force Projection

As soon as you go beyond textbook fixed-base torque computation, inverse dynamics wants another
support layer: how loads applied at frames become generalized forces.

### Unit problems to solve

1. represent external forces as body- or frame-attached spatial wrenches
2. transform them into the body coordinates used by the recursion
3. project them into generalized coordinates when needed
4. expose Jacobians for selected frames and points

### Why this is not optional for a useful library

KDL's inverse-dynamics chain solver includes external segment forces directly in its API.[^kdl-rne]
Drake's inverse-dynamics formulation also makes external applied body forces part of the equation,
not an afterthought.[^drake-multibody]

If this library wants to be useful for contact-aware arms, tool loads, or mobile manipulation,
external-wrench handling is part of the real minimum, not an advanced extra.

### Suggested APIs

```cpp
struct ExternalWrench {
  size_t body_index;
  Wrenchd wrench_in_body;
};

void point_jacobian(...);
void frame_jacobian(...);
void project_external_wrenches(...);
```

This should reuse the existing frame naming and frame-definition infrastructure where possible,
rather than inventing a parallel way to identify attachments.

## Algorithm 6: Actuation Mapping Is A Separate Problem

This is the point where the current joint-map design philosophy becomes especially important.

Rigid-body inverse dynamics computes generalized forces for the mechanism model.
It does **not** automatically solve:

- actuator torque mapping
- reflected inertia
- gear efficiency
- differential or multi-actuator effort allocation
- physically correct effort propagation through every transmission description

Those are actuation problems.

### Why this separation matters in this codebase

The current transmission analysis intentionally avoids assuming that effort should be propagated by
default from mimic-style affine relationships. That is a good sign: the package already recognizes
that kinematic equivalence is not enough to infer physically-correct force mapping.

The same rule should hold here:

- the dynamics core computes generalized forces in model coordinates
- a separate build-time actuation layer maps those to actuator efforts when the transmission
  semantics justify it

### What that secondary layer might look like

Possible names:

- `GeneralizedForceMap`
- `ActuationMap`
- `EffortMap`

Possible API shape:

```cpp
class GeneralizedForceMap {
public:
  void map_generalized_to_actuator_efforts(
    span<const double> generalized_tau,
    span<double> actuator_efforts) const;
};
```

This could depend on `TransmissionAnalysis`, but it should not force the rigid-body recursion
itself to become dynamically extensible in the same way that transmission planning is.

## Algorithm 7: Floating-Base And Mobile-Base Support

This is where the project becomes meaningfully more complex.

For fixed-base serial and branched arms, all generalized coordinates are naturally aligned with
the joint list.

For floating-base systems, several new questions appear:

- does the root have 6 DoF?
- how is orientation represented?
- are `q` and `v` the same dimension?
- is the base unactuated, partially actuated, or constrained?

### Why this matters to your question

You explicitly said "both", and also said the structures should be generalizable.

That means the design should at least avoid blocking floating-base support, even if fixed-base is
implemented first.

### The main design options

#### Option A: fixed-base first, floating-base-aware model later

Start with:

- 1-DoF joints only
- fixed base only
- APIs written so the root is not hard-coded as permanently fixed in the model definition

This is the cheapest path and fits the current code best.

#### Option B: emulate a floating base with six scalar joints

This is structurally simple and fits a library that currently expects one scalar state per joint.
Featherstone's `floatbase` tooling shows this style, while also documenting the singularity
tradeoff and providing special floating-base routines when that tradeoff matters.[^featherstone-software-v2]

The downside is that this is not a clean long-term generalized-coordinate model.

#### Option C: introduce a real free-flyer root representation

This is the most principled long-term path.

Pinocchio and RBDL both support floating-base roots explicitly, but doing this properly usually
means accepting:

- non-Euclidean orientation representation
- `nq != nv` in some cases
- motion-subspace definitions that are not reducible to the current `JointType` enum

That is architecturally bigger, but closer to a serious general-purpose dynamics library.[^pin-joints]
[^rbdl-joints]

### Recommended stance

For this codebase, the best pragmatic position is:

- do **not** design the first dynamics structures in a way that forbids floating-base roots
- do **not** require true floating-base support in the first milestone
- explicitly reserve room in the model representation for root DoFs and joint models with
  `dof_count > 1`

## Algorithm 8: Constrained Dynamics And Contacts

This is beyond basic inverse dynamics, but it is worth calling out because it determines whether
some API choices should be future-proofed now.

If you eventually want:

- contact force solving
- closed loops
- hard constraints
- support polygons / mobile contacts

then you eventually need some variant of:

- Jacobians
- `M(q)`
- bias terms
- a KKT or reduced-space solve

RBDL documents this constrained-dynamics formulation directly.[^rbdl-constraints]

This does **not** mean the first implementation should include constrained dynamics.
It does mean that the early APIs should avoid painting the library into a corner where all output
is assumed to be "joint torques only" and no body/frame wrench infrastructure exists.

## Should Dynamics Be Dynamically Extensible Like Joint Maps?

Probably not in the core recursion layer.

The current joint-map subsystem uses dynamic extensibility because the semantic space of
transmissions is genuinely open-ended:

- different transmission models
- different reachability plans
- different runtime materializations

Rigid-body inverse dynamics is different.

### Why the core should stay closed and data-oriented

The core recursion wants:

- a small number of heavily-optimized kernels
- known memory layout
- minimal virtual dispatch
- well-defined physical invariants

Those are the opposite of the reasons to prefer type-erased runtime extension.

### Where extensibility could still make sense

If extensibility is needed, it is more plausible at build boundaries:

- custom joint model registration
- actuation mapping
- external wrench providers
- constraint builders

Even there, the default should be build-time specialization into compact runtime arrays, not a
runtime polymorphic solver graph.

### Recommendation

For dynamics:

- keep the rigid-body model and recursion layer concrete and closed
- allow higher-level build-time adapters around it if needed
- continue to reuse `JointMap`-style boundary mapping instead of reproducing transmission
  semantics inside the dynamics core

That is more cohesive with the package's current data-oriented design than inventing a "dynamics
plugin architecture" for the low-level kernels.

## Dependency Options

There are several realistic ways to proceed.

### Option 1: Self-contained implementation

Implement:

- rigid-body inertia types and kernels built on top of `Twist`/`Wrench`
- inertialized model build from URDF
- fixed-base RNEA
- gravity and bias extraction
- CRBA
- later Jacobians, ABA, floating-base support

#### Pros

- best fit with current package architecture
- full control over memory layout and API shape
- easiest to make feel like native `arm_kinematics`
- avoids taking a large multibody dependency just to expose a few kernels

#### Cons

- highest implementation and validation burden
- easy to get sign conventions and frame conventions subtly wrong
- floating-base and constraint support become a substantial project

### Option 2: Use RBDL as the core backend

RBDL is much closer to the kind of lean dynamics library that would fit this package than a full
systems framework. It already exposes:

- `InverseDynamics`
- `CompositeRigidBodyAlgorithm`
- `ForwardDynamics`
- floating-base joints
- constraint formulations[^rbdl-dynamics] [^rbdl-joints] [^rbdl-constraints]

#### Pros

- scope is fairly well aligned with this library's needs
- much lighter conceptual footprint than Drake
- useful as a reference implementation even if not adopted permanently

#### Cons

- still requires model translation and API wrapping
- does not naturally inherit the exact analyzed structures already present here
- can tempt the package into becoming a thin adapter rather than a coherent native layer

### Option 3: Use Pinocchio as the core backend

Pinocchio is the most architecturally polished reference for this codebase's preferred style.
Its explicit model/data split is very compatible with the current package philosophy.[^pin-model-data]

It also covers:

- RNEA
- CRBA
- ABA
- floating-base joints
- advanced multibody algorithms[^pin-dynamics] [^pin-joints]

#### Pros

- excellent algorithm coverage
- strong conceptual match for immutable model plus reusable data
- strong floating-base story

#### Cons

- significantly larger dependency and conceptual surface
- Lie-group-heavy API model may be more than this package wants
- likely overkill if the immediate goal is only a compact native inverse-dynamics layer

### Option 4: Use Drake as inspiration, not as the backend

Drake's multibody documentation is useful for equation semantics and system decomposition, and it
has explicit inverse-dynamics APIs with gravity-compensation mode.[^drake-multibody]
[^drake-id]

But as a backend for this package it is likely too heavyweight.

### Option 5: Use KDL only for narrow chain-scoped comparisons

KDL does expose a Recursive Newton-Euler inverse-dynamics solver for chains.[^kdl-rne]
But its own overview notes that tree solvers are limited to forward kinematics only.[^kdl-overview]

That makes it a weak fit if the goal is a generalizable structure for both fixed-base and mobile
or branched systems.

## Recommended Implementation Strategy

The most coherent plan for this repository is:

### Stage 1: Native self-contained fixed-base core

Implement:

- spatial math primitives
- `DynamicsModel`
- `DynamicsData`
- fixed-base 1-DoF joint support
- RNEA
- `gravity_forces`
- `bias_forces`

This is the smallest stage that produces real value while staying architecturally honest.

### Stage 2: Add CRBA and Jacobian support

Implement:

- `mass_matrix`
- `hand_c`
- frame/body Jacobians
- external wrench projection utilities

This stage makes the library broadly useful for model-based control.

### Stage 3: Add actuation mapping as a separate build product

Implement a distinct layer for:

- generalized-force to actuator-effort mapping
- explicit transmission semantics
- opt-in effort projection rules only where physically justified

Do not couple this into the rigid-body recursion.

### Stage 4: Extend the joint/model representation for floating-base support

At that point the team can choose between:

- emulated floating base as an intermediate step
- true free-flyer root support

The right choice depends on whether the real use case is:

- mobile manipulators with a rigidly modeled base
- free-floating systems
- only "potentially generalizable later"

## Concrete API Checklist By Unit Problem

If the question is "what APIs are needed?", the smallest honest answer is:

### Build-time structures

- `DynamicsModel`
- `DynamicsModelBuilder` or free builder functions
- stable body/joint/frame index types

### Runtime state

- `DynamicsData`
- optionally a smaller `KinematicsData` if the project wants to split state layers

### Foundational computations

- `update_kinematics(q, v, a, ...)`
- `frame_jacobian(...)`
- `point_jacobian(...)`

### Inverse-dynamics computations

- `inverse_dynamics(...)`
- `gravity_forces(...)`
- `bias_forces(...)`

### Inertia-related computations

- `mass_matrix(...)`
- `hand_c(...)`

### Force-side boundary utilities

- `project_external_wrenches(...)`
- `GeneralizedForceMap` or equivalent

### Future-facing extensions

- floating-base root representation
- constrained-dynamics solve helpers

## Overall Feasibility Assessment

Inverse dynamics is feasible here **if** it is treated as the next structural layer after FK, not
as a small add-on.

The current package already has:

- the right build/runtime philosophy
- the right preference for compact arrays
- a useful analyzed tree
- a strong precedent for compiling setup-time semantic structure into runtime helpers

The package does **not** yet have:

- an inertial model
- spatial dynamics math
- generalized-force semantics
- multidof root handling

So the realistic conclusion is:

- fixed-base inverse dynamics is very feasible
- a self-contained implementation is plausible and architecturally coherent
- a complete floating-base and constrained-dynamics story is feasible, but should be treated as a
  later expansion
- dynamic extensibility in the style of the transmission subsystem is not the right priority for
  the rigid-body core

If the project wants the most cohesive result, the best path is:

1. implement a native fixed-base dynamics core in the same style as `ComputeJointTree`
2. keep actuator/transmission effort mapping as a separate layer
3. validate against Pinocchio or RBDL during development
4. only then decide whether true floating-base support is worth the additional API complexity

## References

[^pin-model-data]: Pinocchio documentation, "Model and data":
  <https://docs.ros.org/en/rolling/p/pinocchio/doc/a-features/b-model-data.html>

[^pin-dynamics]: Pinocchio documentation, "Dynamics algorithms":
  <https://docs.ros.org/en/rolling/p/pinocchio/doc/a-features/g-dynamic.html>

[^pin-joints]: Pinocchio documentation, "Joints":
  <https://docs.ros.org/en/rolling/p/pinocchio/doc/a-features/c-joints.html>

[^pin-fixed]: Pinocchio documentation note on fixed joints being treated as frames:
  <https://docs.ros.org/en/rolling/p/pinocchio/doc/a-features/c-joints.html>

[^rbdl-dynamics]: RBDL documentation, "Dynamics":
  <https://rbdl.github.io/d6/d63/group__dynamics__group.html>

[^rbdl-joints]: RBDL documentation, "Joint Modeling":
  <https://rbdl.github.io/df/dbe/joint_description.html>

[^rbdl-constraints]: RBDL documentation, "Constraints":
  <https://rbdl.github.io/d3/d7d/group__constraints__group.html>

[^kdl-rne]: Orocos KDL documentation, `KDL::ChainIdSolver_RNE`:
  <https://docs.ros.org/en/kinetic/api/orocos_kdl/html/classKDL_1_1ChainIdSolver__RNE.html>

[^kdl-overview]: Orocos KDL overview:
  <https://docs.orocos.org/kdl/overview.html>

[^drake-multibody]: Drake multibody documentation:
  <https://drake.mit.edu/pydrake/pydrake.multibody.plant.html>

[^drake-id]: Drake `InverseDynamics` documentation:
  <https://drake.mit.edu/doxygen_cxx/classdrake_1_1systems_1_1controllers_1_1_inverse_dynamics.html>

[^featherstone-model]: Roy Featherstone, "The System Model Data Structure":
  <https://royfeatherstone.org/spatial/v2/sysmodel.html>

[^featherstone-software]: Roy Featherstone, "Spatial Vector and Dynamics Software":
  <https://royfeatherstone.org/spatial/index.html>

[^featherstone-software-v2]: Roy Featherstone, `spatial_v2` function index:
  <https://royfeatherstone.org/spatial/v2/>
