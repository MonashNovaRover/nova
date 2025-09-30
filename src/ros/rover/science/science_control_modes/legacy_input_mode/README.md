# legacy_input_mode/LegacyInputMode

Publishes old input types for new teleop.

### Inputs

#### Axes
Axes should be one of the axes within the [old joystick messages](../../../old_inputs/input_interfaces/msg/InputJoystick.msg)

#### Buttons
Buttons should be one of the buttons within the [old joystick messages](../../../old_inputs/input_interfaces/msg/InputJoystick.msg)

### Parameters

- `topic : string` The topic name to send messages to (Required)
    - Should be one of `"/inputs/input_joystick_l"` or `"/inputs/input_joystick_r"`.

```yaml
# Example parameter file
teleop_node:
  ros__parameters:
    control_modes:
      names: [ "legacy_input_mode" ]
      twist_control_mode:
        type: "legacy_input_mode/LegacyInputMode"

legacy_input_mode:
  ros__parameters:
    # Topic to send messages to (Required)
    topic: "/inputs/input_joystick_l"
```
