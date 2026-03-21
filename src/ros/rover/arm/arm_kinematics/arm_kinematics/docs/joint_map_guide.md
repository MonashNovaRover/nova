# JointMap Concept Guide

[`JointMap`](../include/arm_kinematics/joint_map/joint_map.hpp) is one of the most important abstractions in this package, and one of the easiest to misunderstand if you come at it from a controller-only point of view.

The short version is:

A `JointMap` is a reusable mapping from one set of known joint values to another related set of joint values.

Today, that mainly means:

- converting from caller-facing joint ordering to compute-friendly joint ordering
- applying mimic-joint relationships while doing that conversion

In the future, it is also intended to mean:

- propagating values through transmissions
- representing more general "known joints -> related joints" conversions
- allowing plugin-specific builders to generate the right conversion for a particular solver or compute backend

So if someone asks "is `JointMap` just for reordering?", the answer is:

No. Reordering is the simplest and most common case, but the broader conceptual model is value propagation between related joint spaces.

## The Mental Model

The easiest way to understand a `JointMap` is to stop thinking of it as a container of joint names and start thinking of it as a compiled adapter.

It answers this question:

"Given a vector of joint values that I know, how do I produce the vector of joint values that some other part of the system needs?"

That "other part of the system" might be:

- an FK tree
- a collision pose updater
- a plugin-specific solver
- a transmission propagation stage
- a lower-level actuator-facing representation in the future

The map is built once, then reused many times.
That is why it belongs in the setup path, not in the inner control loop.

## Why `JointMap` Exists

In a `ros2_control` controller, the joint order you naturally have is usually the order defined by your interfaces and parameters.
That is a caller-facing order.

But the order a solver wants internally is often different.

Examples:

- an FK compute tree wants joints in topological parent-before-child order
- a specialized solver may want only a subset of joints
- mimic joints may require a derived value rather than a direct input slot
- future transmission propagation may require actuator-space values to be turned into joint-space values, or vice versa

Without a `JointMap`, every runtime computation would need to repeatedly:

- look up names
- find indices
- handle mimic or transmission rules
- build the output vector it really wanted

That is exactly the kind of repeated bookkeeping this library is trying to move out of the hot path.

`JointMap` exists so that all of that structural work can happen once, while runtime use becomes a compact array transform.

## The Core Concept: Input Space And Output Space

Every `JointMap` has:

- an input space
- an output space

The input space is "the values I already have."
The output space is "the values I want to produce."

In today's implementation, the input and output spaces are described primarily by ordered joint name lists when the map is built.

For example:

- input space: controller-facing joints `["shoulder", "elbow", "wrist"]`
- output space: FK tree joints `["elbow", "shoulder"]`

The resulting `JointMap` is the thing that knows how to turn a value vector in the first space into a value vector in the second space.

This is the most important conceptual shift:

The map is not the joints themselves.
The map is the relationship between two joint spaces.

## What The Current Runtime Representation Means

The current `JointMap` runtime representation is intentionally simple:

- `sources`
- `multipliers`
- `offsets`

For each output element `i`, the current implementation computes:

`output[i] = input[sources[i]] * multipliers[i] + offsets[i]`

That means the current implementation can express:

- pure reordering
- copying one input into multiple outputs
- mimic-joint relationships
- simple affine conversions

Examples:

- pure reorder: `output[0] = input[2]`
- copy to two consumers: `output[1] = input[2]`
- mimic: `output[3] = input[0] * -1.0 + 0.5`

This is why a `JointMap` is more than a permutation.
A permutation can only say where values move.
A `JointMap` can also say how a value changes while moving.

## What The Broader Intended Model Is

The package comments and builder structure already point toward a broader meaning.
The intended long-term model is:

A `JointMap` represents known-joint-value propagation across related kinematic representations.

That includes:

- joint ordering conversion
- mimic-joint derivation
- transmission propagation
- plugin-specific derived-joint generation

In other words, the current `sources`/`multipliers`/`offsets` form is the current concrete implementation, not the full conceptual limit of the abstraction.

That distinction matters when explaining the library:

- conceptually, `JointMap` is the library's joint-space conversion abstraction
- currently, the default implementation is an efficient affine/gather mapping

## Where `JointMap` Comes From

You normally do not hand-author a `JointMap`.
You ask a [`JointMapBuilder`](../include/arm_kinematics/joint_map/joint_map_builder.hpp) to build one.

The default source of that builder is usually:

- the shared [`RobotModel`](../include/arm_kinematics/common/robot_model.hpp), or
- an FK plugin's `get_joint_map_builder()`

That matters because different plugin implementations may want different joint-space rules.

Today, the default FK base class returns the `RobotModel`'s standard builder.
A specialized plugin could override that and provide a builder that knows how to generate joint maps suitable for its own compute representation.

So the conceptual ownership is:

- `JointMap` is the compiled mapping
- `JointMapBuilder` is the factory that understands how to create that mapping
- a plugin may be the right place to expose the default builder for its own solver backend

## How You Get A JointMap

### From `RobotModel`

If you have a shared `RobotModel`, you can get the default builder from it:

```cpp
const auto & builder = robot_model.get_joint_map_builder();
auto map = builder.build(input_joint_names, output_joint_names);
```

This is the most direct way to say:

"I know my controller's joint order, and I know the target order I need. Build me the mapping."

### From an FK plugin

If you already have an FK plugin, the plugin is often the best source of the builder because it is the plugin that knows what joint order its compute structures expect:

```cpp
const auto & builder = fk_plugin->get_joint_map_builder();
auto map = builder.build(controller_joint_names, internal_joint_names);
```

This is the right mental model:

- the caller owns the input space
- the plugin/backend owns the output space
- the builder creates the bridge between them

### As part of FK tree construction

In the default Eigen FK implementation, you usually do not build the map manually because the plugin builds it while constructing its tree.

Conceptually, the plugin does something like:

```cpp
auto subtree = AnalysisTree(robot_model.get_analysis_tree(), base_link_name, frames);
subtree.sort_joints();

std::vector<std::string> internal_joint_names{
  subtree.get_joints().names.begin() + 1,
  subtree.get_joints().names.end()
};

auto map = joint_map_builder.build(caller_joint_names, internal_joint_names);
```

That is exactly the caller-facing-ordering to compute-friendly-ordering use case.

## When You Use A JointMap

There are two broad categories of use.

## 1. Interfacing with the library

This is the most common use today.

You use a `JointMap` whenever:

- your controller has joint values in one order
- the library component you want to use expects them in another order

Typical cases:

- FK tree updates
- collision pose updates through an FK tree
- any plugin with an internal joint layout that is not the controller's layout

The pattern is:

1. build the map during setup
2. preallocate the mapped output buffer
3. call `map()` each cycle before updating the compute structure

Example:

```cpp
std::vector<std::string> controller_joint_names{
  "shoulder",
  "elbow",
  "wrist"
};

std::vector<std::string> fk_joint_names{
  "elbow",
  "shoulder"
};

auto map = robot_model.get_joint_map_builder().build(
  controller_joint_names,
  fk_joint_names);

std::vector<double> controller_positions{1.2, 0.4, -0.1};
std::vector<float> fk_positions(map.output_count);

map.map(controller_positions, fk_positions);
```

After mapping:

- `fk_positions[0]` contains the controller's `"elbow"` value
- `fk_positions[1]` contains the controller's `"shoulder"` value

The FK code never needs to care what the original controller ordering was.

## 2. General propagation of related joint values

This is the broader use case and the one worth teaching explicitly, because it explains why `JointMap` is not just a reorder helper.

You use a `JointMap` whenever:

- you know one set of joint-like values
- another set of related joint-like values should be derivable from them
- you want that derivation represented explicitly and reused

Today this is mainly mimic joints.
In the future this should also cover transmission propagation and other plugin-specific related-joint generation.

For example, imagine:

- you know actuator-side values
- your solver wants joint-side values

Or:

- you know primary joint values
- a mimic joint should be derived automatically

Conceptually those are the same pattern:

- one known space
- one desired related space
- one reusable map between them

That is why the abstraction is valuable even beyond FK.

## How You Use A JointMap

The usage pattern is intentionally simple.

### Step 1: build it once

```cpp
auto map = builder.build(input_joint_names, output_joint_names);
```

### Step 2: allocate the output buffer once

```cpp
std::vector<float> mapped_values(map.output_count);
```

Or if you are targeting KDL:

```cpp
KDL::JntArray mapped_values(map.output_count);
```

### Step 3: apply it whenever new inputs arrive

```cpp
map.map(input_values, mapped_values);
```

That is the whole runtime contract.

The important practical rule is:

The output buffer must already have the correct size.
`JointMap` is designed to fill preallocated storage, not to resize vectors in your realtime path.

## A More Concrete Example: Mimic Joint Behavior

Suppose your caller knows:

- `["finger_driver"]`

But your output space needs:

- `["finger_driver", "finger_follower"]`

And the follower is defined as:

- `finger_follower = finger_driver * -1.0 + 0.25`

Then the resulting conceptual map is:

- output `"finger_driver"` comes directly from input `"finger_driver"`
- output `"finger_follower"` is derived from input `"finger_driver"`

That is still a `JointMap`.
It is not a reorder anymore.
It is propagation of known values into a related joint space.

This is usually the clearest example to show people that `JointMap` is broader than permutation.

## A More Concrete Example: FK Tree Integration

This is the main practical use inside the library.

The controller might expose:

```cpp
std::vector<std::string> joint_names{
  "joint_a",
  "joint_b",
  "joint_c"
};
```

But after the FK plugin builds a reduced, sorted compute tree, the internal joint order it needs might be:

```cpp
std::vector<std::string> internal_joint_names{
  "joint_b",
  "joint_a"
};
```

The plugin creates:

```cpp
auto map = builder.build(joint_names, internal_joint_names);
```

At runtime:

```cpp
std::vector<float> internal_joint_values(map.output_count);
map.map(controller_joint_values, internal_joint_values);
tree.update(internal_joint_values, output_poses.data());
```

That is the conceptual handoff:

- the controller speaks in controller order
- the compute tree speaks in compute order
- `JointMap` is the translation layer

## A Future-Facing Example: Transmission Propagation

This is not fully realized in the current default implementation, but it is the right conceptual direction.

Imagine a future specialized builder where:

- the inputs are actuator-reported positions
- the outputs are the joint values needed by a solver
- transmission ratios and offsets are embedded in the mapping

Then the usage would still look conceptually the same:

```cpp
auto joint_map = plugin_specific_builder.build(
  actuator_names,
  solver_joint_names);

joint_map.map(actuator_values, solver_joint_values);
```

The point is that the caller does not need to know the propagation details every cycle.
Those details are compiled into the map during setup.

This is why it makes sense to describe `JointMap` as a general related-joint conversion abstraction, not just a reorder helper.

## How To Explain It In One Sentence

If you need a sentence for other developers, this is a good one:

A `JointMap` is a precomputed adapter that turns one ordered set of known joint values into another ordered set of related joint values.

If you need the slightly longer version:

A `JointMap` lets the library separate the joint order and semantics that are convenient at the API boundary from the joint order and semantics that are convenient for the internal compute backend.

## Common Misunderstandings

### "It is just a permutation."

Not quite.
It can represent permutation, but it can also represent derived outputs such as mimic joints, and it is intended to grow into broader transmission-related propagation.

### "It stores the joint state."

No.
It stores the rules for transforming one joint-value vector into another.

### "It belongs only to FK."

No.
FK is the clearest current use case, but the abstraction is really about converting between related joint spaces generally.

### "It is only needed because the API is awkward."

No.
Even with a perfect API, compute backends often want a different subset, order, or derived representation than the controller boundary naturally provides.

## Practical Guidance

When deciding whether you need a `JointMap`, ask:

1. Do I have joint values in one space and need them in another?
2. Is the relationship stable enough to precompute once?
3. Will I apply that relationship repeatedly at runtime?

If the answer is yes, you probably want a `JointMap`.

When explaining `JointMap` to others, emphasize these three ideas:

1. It separates caller-facing representation from compute-facing representation.
2. It is about propagation of related values, not just reordering.
3. It is built once and reused many times.

That is the conceptual model this package is aiming for.
