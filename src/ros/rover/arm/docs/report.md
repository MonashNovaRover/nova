


# State Interface Acquisition Utilities — Research & Design

## Problem Statement

Every arm controller that wants to feed joint state into `arm_kinematics` tools must independently
implement the same boilerplate:

1. **Build interface name strings** for `InterfaceConfiguration` (e.g. `"shoulder_pitch/position"`)
2. **Search `state_interfaces_`** to locate each joint × type pair
3. **Read their values** into a contiguous buffer each update tick

None of these are shared. The pattern is copy-pasted across `nova_arm_controller`,
`nova_twistmapper`, and `nova_path_planner`, with divergences in each.

### Concrete examples of the current mess

**Manual string construction** appears in every controller's `state_interface_configuration()`
and `configure_joints()`:

```cpp
// nova_arm_controller.cpp:77
conf_names.push_back(joint_name + "/" + HW_IF_POSITION);
```

**Divergent search strategies** — `nova_arm_controller` matches prefix + interface name
separately, `nova_twistmapper` constructs a full name and matches `get_name()`:

```cpp
// nova_arm_controller.cpp:571 — matches prefix + interface name
const auto pos_state_handle = std::find_if(
    state_interfaces_.cbegin(), state_interfaces_.cend(),
    [&joint_name](const auto &interface) {
      return interface.get_prefix_name() == joint_name &&
             interface.get_interface_name() == HW_IF_POSITION;
    });

// nova_twistmapper.cpp:587 — constructs full name then matches get_name()
const auto state_interface_name = joint_name + "/" + hardware_interface::HW_IF_POSITION;
const auto state_handle = std::find_if(
    state_interfaces_.begin(), state_interfaces_.end(),
    [&state_interface_name](const auto &interface) {
      return interface.get_name() == state_interface_name;
    });
```

**Verbose failure logging** reproduced in every controller:

```cpp
// nova_twistmapper.cpp:600
RCLCPP_ERROR(logger, "state_interfaces_:");
for (const auto& state_interface : state_interfaces_) {
  RCLCPP_ERROR(logger, "  > interface_name: %s", state_interface.get_interface_name().c_str());
  RCLCPP_ERROR(logger, "    prefix_name: %s",    state_interface.get_prefix_name().c_str());
  RCLCPP_ERROR(logger, "    name: %s",            state_interface.get_name().c_str());
}
```

**Value extraction loop** re-implemented in each controller:

```cpp
// nova_twistmapper.cpp:638
std::vector<double> NovaTwistmapper::get_state_pos_values() {
  std::vector<double> joint_values;
  joint_values.reserve(registered_joint_handles_.size());
  for (auto& joint_handle : registered_joint_handles_)
    joint_values.emplace_back(joint_handle.state_pos.get().get_value());
  return joint_values;
}
```

---

## Background

### How arm_kinematics works

There is **no global joint ordering** that all tools share. Each runtime compute structure
(FK tree, collision checker, etc.) owns its own `JointMap`.

A `JointMap` is built via `JointMapBuilder::build_expected(inputs, outputs)`:

```cpp
class JointMapBuilder {
  [[nodiscard]] virtual tl::expected<JointMap, JointMapBuildError> build_expected(
    span<const StateInterfaceDefinition> inputs,
    span<const StateInterfaceDefinition> outputs) const = 0;
};
```

- `inputs`: the ordered list of state interface values the controller will supply
- `outputs`: the ordered list of state interface values the map will produce
- The builder handles transmissions and mimic joints automatically
- Fails with `JointMapBuildError` (listing unproducible outputs and resolution hints) if
  outputs cannot be derived from inputs

At runtime:

```cpp
class JointMap {
  void map(span<const double> inputs, span<double> outputs) const;
};
```

`inputs` must be in the **same order** as the `inputs` span passed to `build_expected()`.
The controller is responsible for assembling this buffer.

`TransmissionAnalysis::joint_order()` is an `Order<std::string, JointId>` — a bidirectional
map for converting joint name strings to compact `JointId` integers and back. It is **not**
the ordering that runtime tools expect; it is a fast lookup structure for resolving names
to IDs within the analysis.

### ros2_control API surface

Every controller has:

```cpp
std::vector<hardware_interface::LoanedStateInterface> state_interfaces_;
```

populated by the controller manager after `state_interface_configuration()` declares which
interfaces are needed.

Each `LoanedStateInterface` exposes:

| Method | Returns | Example |
|---|---|---|
| `get_name()` | full interface name | `"shoulder_pitch/position"` |
| `get_prefix_name()` | joint name | `"shoulder_pitch"` |
| `get_interface_name()` | interface type | `"position"` |
| `get_optional<double>()` | `std::optional<double>` | modern, thread-safe, non-deprecated |
| `get_value()` | `double` | **deprecated** — returns `NaN` on failure |

`InterfaceConfiguration` is declared by each controller as:

```cpp
struct InterfaceConfiguration {
  interface_configuration_type type;   // INDIVIDUAL, ALL, or NONE
  std::vector<std::string> names;      // "joint_name/interface_type" strings
};
```

### Bridging the gap

`StateInterfaceDefinition` uses `JointId` (size_t) + `InterfaceId` (hash + name string).
`LoanedStateInterface` uses human-readable strings. To connect them:

1. `TransmissionAnalysis::joint_order()` converts `JointId` → joint name string
2. A string search into `state_interfaces_` finds the matching `LoanedStateInterface`

This string work belongs entirely at **configure time**. The RT update loop should do nothing
but iterate an already-resolved list of refs.

---

## Design Principles

- **Minimal and self-contained at each layer.** Each utility does one thing with no
  unnecessary dependencies.
- **Higher level utilities tie lower ones together** to enable more complex but automatic
  utilities — but there is no single top-level god utility. Each utility is independently
  useful.
- **Resolve complexity at configuration time; do the cheapest possible thing at runtime.**
  String lookups and resolution happen once in `on_configure()`/`on_activate()`. The RT
  `update()` loop only iterates pre-resolved `reference_wrapper` refs.
- **Fail loudly at configuration time.** Errors surface before the controller is active,
  not silently during `update()`.
- **Prefer `span<>` over owning memory** when the caller can reasonably own the buffer —
  but don't force ownership onto a utility that would otherwise be simpler without it.

---

## Proposed Utilities

Three narrow, header-only files under:

```
arm_kinematics/arm_kinematics/include/arm_kinematics/ros2_control/
```

---

### Utility A — Interface name strings

**File:** `state_interface_names.hpp`

Generates the `"joint/type"` strings required by `InterfaceConfiguration::names`.
Accepts `NamedStateInterfaceDefinition` directly so the same declaration the controller
passes to `make_tree()` can be reused without re-stating the joint list.

```cpp
namespace arm_kinematics::ros2_control {

// Single definition: "shoulder_pitch/position"
std::string state_interface_name(const NamedStateInterfaceDefinition & def);

// List of definitions → list of "joint/type" strings
std::vector<std::string> state_interface_names(
    span<const NamedStateInterfaceDefinition> defs);

// Joint names × interface types (for cases where a NamedStateInterfaceDefinition
// list isn't already available)
std::vector<std::string> state_interface_names(
    span<const std::string>                 joint_names,
    std::initializer_list<std::string_view> types);

} // namespace arm_kinematics::ros2_control
```

Dependencies: `arm_kinematics/joint_map/state_interface_definition.hpp` and `<string>`.

---

### Utility B — Ordered ref acquisition

**File:** `ordered_state_interface_refs.hpp`

Given an ordered list of `StateInterfaceDefinition`s (the same `inputs` the controller
passed to `build_expected()`) and the controller's `state_interfaces_`, resolve each
definition to a `LoanedStateInterface` ref in the same order. This is the configuration-time
counterpart to the runtime `map()` call — done once, cheap forever after.

```cpp
namespace arm_kinematics::ros2_control {

using LoanedStateRef =
    std::reference_wrapper<const hardware_interface::LoanedStateInterface>;

struct MissingStateInterface {
    StateInterfaceDefinition definition;
    std::string joint_name; // resolved from JointId — for human-readable error messages
};

// Resolve definitions → ordered refs.
// analysis is needed to convert JointId → joint name string.
// Returned refs[i] corresponds to definitions[i].
tl::expected<
    std::vector<LoanedStateRef>,
    std::vector<MissingStateInterface>>
ordered_state_interface_refs(
    std::vector<hardware_interface::LoanedStateInterface> & state_interfaces,
    span<const StateInterfaceDefinition>                    definitions,
    const TransmissionAnalysis &                            analysis);

// Convenience overload accepting NamedStateInterfaceDefinition.
// Resolves names → JointIds via analysis, then delegates to the above.
tl::expected<
    std::vector<LoanedStateRef>,
    std::vector<MissingStateInterface>>
ordered_state_interface_refs(
    std::vector<hardware_interface::LoanedStateInterface> & state_interfaces,
    span<const NamedStateInterfaceDefinition>               definitions,
    const TransmissionAnalysis &                            analysis);

} // namespace arm_kinematics::ros2_control
```

The string search over `state_interfaces_` happens here, once at configure time.
The result is a `vector<reference_wrapper<...>>` that is cheap to iterate at runtime.

Dependencies: `tl/expected.hpp`, `hardware_interface/loaned_state_interface.hpp`,
`arm_kinematics/joint_map/state_interface_definition.hpp`,
`arm_kinematics/joint_map/transmission_analysis.hpp`.

---

### Utility C — Value reading

**File:** `read_state_interface_values.hpp`

Reads current values from an ordered ref list into a caller-supplied buffer.
This is the only operation that runs in the RT `update()` loop.
Uses `get_optional<double>()` (the modern, non-deprecated API).

```cpp
namespace arm_kinematics::ros2_control {

using LoanedStateRef =
    std::reference_wrapper<const hardware_interface::LoanedStateInterface>;

// Write current interface values into out[i] for each refs[i].
// out.size() must equal refs.size().
void read_state_interface_values(
    span<const LoanedStateRef> refs,
    span<double>               out);

} // namespace arm_kinematics::ros2_control
```

Dependencies: `hardware_interface/loaned_state_interface.hpp`, `<span>`.

---

## Usage Pattern

```cpp
// ── Declare inputs (same object reused for make_tree, InterfaceConfiguration, and ref lookup) ──
const std::vector<NamedStateInterfaceDefinition> state_inputs = {
    {"shoulder_pitch", InterfaceId("position")},
    {"elbow",          InterfaceId("position")},
};

// ── state_interface_configuration() [Utility A] ──
InterfaceConfiguration MyController::state_interface_configuration() const {
    return {interface_configuration_type::INDIVIDUAL,
            state_interface_names(state_inputs)};
}

// ── on_activate(), after assign_interfaces [Utility B] ──
auto refs_result = ordered_state_interface_refs(state_interfaces_, state_inputs, analysis_);
if (!refs_result) {
    for (const auto & missing : refs_result.error())
        RCLCPP_ERROR(logger_, "Missing state interface: %s/%s",
                     missing.joint_name.c_str(),
                     missing.definition.interface_id.name.c_str());
    return CallbackReturn::ERROR;
}
state_refs_ = std::move(refs_result).value();
input_buffer_.resize(state_refs_.size());

// ── update() [Utility C → JointMap] ──
read_state_interface_values(state_refs_, input_buffer_);
joint_map_.map(input_buffer_, output_buffer_);
fk_tree_->position_fk(output_buffer_, link_poses_);
```

**Before / after summary:**

| Step | Current | With utilities |
|---|---|---|
| `InterfaceConfiguration` | Manual string concat loop | `state_interface_names(state_inputs)` |
| Acquire refs | `std::find_if` × N, divergent across controllers | `ordered_state_interface_refs(...)` |
| Error reporting | Dumps entire `state_interfaces_` list | Lists exactly what's missing, by name |
| Read values | `get_value()` (deprecated) loop, reimplemented per controller | `read_state_interface_values(refs, buf)` |
| Ordering | Relies on param order matching kinematics order (fragile) | Explicitly matches the `inputs` list from `build_expected()` |

---

## What Is Not In Scope

- **Wrapping `controller_interface::get_ordered_interfaces`** — it operates on strings and
  doesn't compose well with `StateInterfaceDefinition`/`JointId`-based tooling.
- **Auto-discovering needed state interfaces from `JointMapBuildError::resolutions`** — the
  appropriate response to a build failure is to loudly report the resolutions and ask the
  user to add the missing joints to a controller parameter. This is simpler and more explicit.
- **Command interface utilities** — out of scope; only state interfaces are targeted here.
- **A god utility** combining all three steps — the three utilities are independently useful
  and should remain separate.

---

## Open Questions

**`LoanedStateRef` duplication across files.** Both `ordered_state_interface_refs.hpp` and
`read_state_interface_values.hpp` define or use `LoanedStateRef`. This alias could live in a
shared `ros2_control/types.hpp`, or be left inline in each file since both files are small.
Preference TBD.

**`NamedStateInterfaceDefinition` overload in Utility B.** The overload that accepts
`NamedStateInterfaceDefinition`s must internally resolve names → `JointId`s via
`TransmissionAnalysis::joint_order()`. If a name is not found in the analysis, should that
be a `MissingStateInterface` error or a separate error type (unknown joint)? Simplest answer:
treat it as missing (can always be expanded later).
