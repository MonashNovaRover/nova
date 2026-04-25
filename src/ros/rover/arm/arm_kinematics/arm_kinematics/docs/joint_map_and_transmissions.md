# Joint Maps And Transmissions

This guide covers the setup and build-time structures behind joint-space remapping in
`arm_kinematics`.

These types matter even if controller code never touches them directly, because FK tree
construction and collision setup depend on them.

## What Problem These Types Solve

Controllers usually think in terms of named joints in controller order:

```text
["shoulder", "elbow", "wrist"]
```

Kinematics and transmission logic often need something more structured:

- stable internal joint ids
- explicit interface types such as position or velocity
- derived relationships such as mimic or other transmissions
- a compact runtime object that maps one ordered vector into another

`arm_kinematics` splits that problem into analysis, build, and runtime layers instead of trying
to do everything in one object.

That split exists to keep two concerns separate:

- structural reasoning
  names, ids, reachability, mimic relationships, transmission policy
- runtime execution
  fast ordered-vector transforms with minimal branching and no repeated structural work

## API Boundary Types

### NamedStateInterfaceDefinition

`NamedStateInterfaceDefinition` is the convenient controller-facing form:

```cpp
{"joint_name", arm_kinematics::InterfaceId::Position()}
```

Use it when your code still works in terms of joint names, for example:

- declaring the state inputs for an FK tree
- looking up state interface refs in a controller
- building user-facing error messages

### StateInterfaceDefinition

`StateInterfaceDefinition` is the id-based form:

```cpp
{joint_id, arm_kinematics::InterfaceId::Position()}
```

Use it after names have already been resolved against a `TransmissionAnalysis`.

This is the canonical request shape for `JointMapBuilder::build_expected(...)`.

Why the package has both forms:

- names are easier and safer at controller boundaries
- ids are cheaper and less ambiguous once analysis has already resolved the names

## Analysis Layer: TransmissionAnalysis

`TransmissionAnalysis` is the build-time graph of joint and interface relationships.

It owns:

- the mapping from joint names to stable `JointId`s
- the mapping from registered `(JointId, InterfaceId)` entries to stable state-interface ids
- registered transmission models and transmission instances
- normalized affine relationships between joints

That second mapping is intentionally not a registry of every valid `(JointId, InterfaceId)` pair.
It only contains state interfaces that have actually been registered in the analysis. In practice,
that often means interfaces that participate in affine or transmission relationships, not every
interface that could exist in principle for a joint.

This type is about meaning and reachability, not runtime execution.

Important consequences:

- it is append-only during setup
- it is not a hot-path runtime object
- it is the place where semantic relationships are recorded
- it should be populated before you ask builders to produce runtime maps

For FK-related work, `ForwardKinematicsPlugin::get_transmission_analysis()` is the source of truth
and should be preferred. `RobotModel::get_default_transmission_analysis()` is the shared default
analysis, but a concrete FK plugin may expose a different or extended analysis and callers should
not assume the robot-model default is authoritative for that plugin.

Why you would use it directly:

- to build custom mapping or planning flows that need stable ids from the same analysis the FK
  plugin is using
- to inspect what joints and interfaces are actually reachable
- to keep semantic ownership of transmissions in one place instead of scattering rules through
  runtime code

## Build Layer: JointMapBuilder

`JointMapBuilder` turns a mapping request into a runtime `JointMap`.

The request is:

- a list of input `StateInterfaceDefinition`s
- a list of output `StateInterfaceDefinition`s

The answer is either:

- a reusable `JointMap`
- or a structured `JointMapBuildError`

The builder is where planning happens:

- resolve which outputs are reachable from which inputs
- apply transmission policy
- detect unknown joints, ambiguity, or missing inputs
- compile the result into a runtime mapper

This is not real-time safe. Build once and reuse the result.

Why you would use a builder instead of hand-writing vector shuffles:

- transmission reachability is more than simple reordering
- structured errors are better than silent wrong mappings
- the result can be compiled once and reused many times

## Runtime Layer: JointMap

`JointMap` is the compact runtime object that maps one ordered joint-space vector into another.

At runtime it only does three things:

- accept an ordered input span
- write an ordered output span
- report its input and output sizes

That separation is deliberate:

- `TransmissionAnalysis` owns relationships
- `JointMapBuilder` plans the mapping
- `JointMap` executes the mapping

Most runtime code should depend only on the final `JointMap`, not on transmission analysis internals.

Why you would use `JointMap` directly:

- you need to adapt between controller order and algorithm order
- you need that adaptation to be cheap enough for repeated runtime use
- you do not want name lookup or transmission reasoning in the hot path

## Affine And Mimic Relationships

Mimic-style relationships are represented in the analysis/build layers as affine relationships.

The important rule for maintainers is:

- mimic semantics should originate in analysis
- runtime code should consume a prebuilt map or tree that already reflects those relationships

In practice this means:

- `TransmissionAnalysis` stores normalized affine relationships
- builders use those relationships when producing a `JointMap`
- FK tree construction can then consume the built map instead of re-deriving mimic behavior at
  runtime

This design avoids a common footgun: representing mimic semantics independently in multiple
runtime objects and then debugging why FK, collision, and controller-side remapping disagree.

## Where FK Uses These Types

FK tree construction is the most common place where controller-facing code meets this subsystem.

When controller code calls:

```cpp
fk->make_tree(named_inputs, base_link_name, frames);
```

the typical flow is:

1. named inputs are resolved against the FK plugin's transmission analysis
2. the plugin uses a `JointMapBuilder` to build the joint-space mapping it needs
3. the resulting `JointMap` is embedded into the produced FK tree
4. runtime `position_fk(...)` calls reuse that prebuilt mapping

The important ownership rule is that the FK plugin's transmission analysis drives this whole
pipeline. If controller or setup code separately consults `RobotModel::get_default_transmission_analysis()`,
that is only safe if the plugin is known to use that same analysis unchanged.

This is why `make_tree(...)` is setup work, while `Tree::position_fk(...)` is runtime work.

## Common Footguns

- Building maps on demand inside runtime code
  `JointMapBuilder` does analysis and allocation work and is not intended for hot paths.
- Passing name-based data too far into low-level code
  Repeated name resolution is slower and makes ordering bugs harder to see.
- Letting different subsystems invent their own mapping logic
  If FK, collision, and controller code each remap joints separately, drift is likely.

## Practical Guidance

- Use `NamedStateInterfaceDefinition` in controllers and other name-based code.
- Use `StateInterfaceDefinition` only after names have been resolved and you are explicitly
  working at analysis/build level.
- Use `JointMapBuilder` during initialization, never in the hot path.
- Pass around `JointMap` as the runtime execution object.
- If you are extending the package, keep semantic ownership in `TransmissionAnalysis` and keep
  runtime objects small and allocation-free.
