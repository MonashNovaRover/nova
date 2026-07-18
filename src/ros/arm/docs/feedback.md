# There is no global order in which runtime/compute structures expect joint values to be provided.

Instead, different runtime/compute structures expect their own unique order of state interface values to be provided,
and we already have the utilities to map known state interface values to these desired ordered state interface values
respecting transmissions, and failing when an output value cannot be inferred from the provided inputs.

> The canonical joint ordering is `TransmissionAnalysis::joint_order()`, a bidirectional
> `Order<std::string, JointId>` map. **Callers must match this order**, and currently none do
> so explicitly — they rely on parameter order being the same, which is fragile.

False! `TransmissionAnalysis::joint_order()` is the canonical joint ordering within the `TransmissionAnalysis`.
It defined some `size_t` (aliased as `JointId`) value for each joint in the URDFs string name.
We can use this to speed things up by converting expensive strings into cheap numerical IDs when working with joints.

I am ok with you using `TransmissionAnalysis::joint_order()` to convert strings into `size_t` (aliased as `JointId`) 
values.

However, TransmissionAnalysis is an analysis structure. The order in which values are provided to runtime/compute 
structures from arm_kinematics has absolutely nothing to do with `TransmissionAnalysis::joint_order()`. 

- Runtime/compute structures often work on state interface definitions, not joints.
- Each runtime/compute structure works on its own `Order<>`.
- While you could use the lower level structures that deal with `Order<>`, I wouldn't expect you to do this from a 
  controller

Reordering values is NOT our responsibility. The library already defines the `JointMap` utility already. 

- Built from a list of desired input and output `StateInterfaceDefinition`s
- Builds can fail if a value in the desired outputs cannot be found in inputs, nor derived from values in inputs via 
  transmissions.

Please look at source files for the appropriate utilities.

```c++
// See joint_map_builder.hpp
// The default implementation of the interface is found at default_joint_map_builder.hpp/.cpp
class JointMapBuilder {
  [[nodiscard]] virtual tl::expected<JointMap, JointMapBuildError> build_expected(
    span<const StateInterfaceDefinition> inputs,
    span<const StateInterfaceDefinition> outputs) const = 0;
};
```

```c++
// See joint_map.hpp
// Type-erased.
class JointMap {
  void map(span<const double> inputs, span<double> outputs) const;
}
```

Controllers using the arm_kinematics library might have many different runtime/compute structures, with different joint 
maps that will need to be built with enough inputs as to sufficiently define the output joints for all the joint maps.

Controllers will often have a working set of joints, that aren't just used for state interfaces. nova_arm_controller for 
example, accepts some list of joints as control values. If it wanted to use the library for collision detection, it 
would want to use the position values provided as part of that command message for its command joints, but also supply 
any necessary (but no more than necessary!) auxiliary state interface values needed by some colliders.

Our goal is not to solve every problem, but to provide reusable, flexible and useful tools for solving the kinds of 
problems we expect to solve when writing controllers.

No single controller should necessarily need to provide the values for literally every single joint on a controller. 
Even if those joint are defined within a TransmissionAnalysis. My robotic arm does not need to care about the speed of 
some wheel, for example.

# Design principle issues.

> **Higher layers compose lower ones.** top-level utility is just glue; all logic lives
> in reusable primitives.

This wording implies there is a single top-level god utility. That is not the goal. We just want to provide the tools 
for people to do what they want without frustration over the minutia, and with clean code. We don't need to abstract 
away literally everything.

> **Zero-allocation hot path.** Value extraction in `update()` writes into a caller-provided
> `std::span<double>` — no heap allocation per tick.

While it is preferred to operate on `span<>`s instead of owning memory, that doesn't mean owning memory is banned. I 
often just treat memory ownership as another responsibility. Our minimal responsibility principle then informs that we
don't force memory to be owned by a structure that would otherwise be useful in contexts where it is working on memory 
that it doesn't own. Generally, I like this rule, I just wanted to clarify *why* we typically use `span<>`. It is more 
of a guideline than a rule, and can be broken where the design would be better.

> **Fail loudly at configuration time.** Errors surface during `on_configure()` / `on_activate()`,
> not silently during `update()`.

Yes! 

Do be aware, though, we might have utilities for providing values from alternative sources (such as from /joint_states) 
in the future.

- **Reuse what exists.** `controller_interface::get_ordered_interfaces` is wrapped, not
  reimplemented.

I disagree. Generally I agree with the DRY principle. However, `controller_interface::get_ordered_interfaces` is not 
always the appropriate utility to use! 

`controller_interface::get_ordered_interfaces` operates on strings, and does string lookup. We have the `Order<>`s and 
other utilities that help us escape string lookup.

# We are jumping the gun

Don't propose solutions until we have a solid understanding of the problem.

We should 
