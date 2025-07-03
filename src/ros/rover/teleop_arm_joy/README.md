# teleop

`teleop` is a generalized framework for teleoperation input handling in ROS2. It pairs well with `ros2_control`, but is 
generalized to be useful for teleoperation with any system. 

It was written by Bailey Chessum at the end of the 2024-25 design cycle.

## Inputs

TODO: Write

## Input Sources

Input sources provide input values. This could be a joystick, a keyboard, etc. Anything at all.

TODO: Write

### How input sources work

Each input source has its own node, with the same name as the input source name defined in your `teleop` node's 
parameter file, that can be used to:
  - Declare and get parameters to configure the input source
  - Create topic subscriptions, services, etc.
The node is spun on its own separate thread to input updates. You can store any data you receive in a member variable 
during topic subscription callbacks, then call `InputSource::request_update(rclcpp::Time now)` to have the main input 
thread call your `InputSource` implementation's `update(rclcpp::Time now)` method.

If you don't use a ROS2 topic to get the input values for your `InputSource`, you can also start up your own thread in
`TODO: Determine the appropriate place to allow this`, and clean up the thread in `TODO: make a end of lifecycle virtual
method`.

### Input remapping

`teleop` makes it easy to write new input sources, that are feature-rich with minimal implementation. `teleop` was 
written with the idea that to share complex input remapping functionality across many input sources, it should be 
implemented once as part of big `teleop`'s input management scheme.

To get comprehensive input remapping for a custom input source, all your input source needs to do is export a set of 
`{std::string, double&}` pairs for axes and `{std::string, bool&}` for buttons. This is done in the 
`export_buttons(std::vector<InputDeclaration<bool>>& declarations)` and 
`export_axes(std::vector<InputDeclaration<double>>& declarations)` methods in your `InputSource` implementation. 
A new input source is feature-rich and configurable by default.

```yaml
joy_input_source:
  ros__parameters:
    # For example, JoyInputSource simply exports everything in these arrays:
    axis_definitions: [
      "left_stick_x",
      "left_stick_y",
      "right_stick_x",
      "right_stick_y",
      "right_trigger"
    ]
    button_definitions: [
      "lock",
      "unlock"
    ]
```

`teleop`'s `InputSourceManager` will declare parameters allowing you to define meaningfully named inputs from the names 
exported by your `InputSource` implementation.

```yaml
joy_input_source:
  ros__parameters:
    # Defined by InputSourceManager for all InputSources
    remap:
      axes:
        J1:                     # Names used by ControlModes,
          from: "left_stick_x"  #   map to names exported by the InputSource.
        J2:
          from: "left_stick_y"
        J3:
          from: "left_stick_x"
        J4:
          from: "left_stick_y"
          # Clamp input value to a range
          range:                
            in: [-1, 1]
            limit: true  
        speed:
          from: "right_trigger"
          # Remap the range of the input from [-1, 1] to [0, 1].
          range:
            in: [-1.0, 1.0]
            out: [0.0, 1.0]
        J5:
          # Axes can be made from buttons!  
          from_buttons: 
            negative: "dpad_left"
            positive: "dpad_right"
            default_value: 0.0
          # Pressed buttons will output the range bounds, with this being the default:
          # range:
          #   in: [-1.0, 1.0]
          #   out: [-1.0, 1.0]
      buttons:
        lock:
          from: "start"
        unlock:
          from: "share"
  
```

![Example mapping of inputs in a diagram](./docs/big_teleop_input_mapping_example_light.drawio.svg)

Remapping is entirely optional. If you don't set the parameters in `remap` for an input, the `Button` or `Axis` will 
just use the provided bool/double reference directly. So, beyond declaring parameters, there is no additional overhead. 

Similarly, any remapping definitions that only set the `.from` parameter will incur no computational overhead.

### Providing inputs with ROS2

Not all input sources benefit from a system like an `InputSource`. You might just want to write slop for a prototype. You might have a system that could provide a sparse set of inputs very occasionally, such 
as buttons on a web-based GUI. To support this, the `teleop` node will expose services that allow you to directly set 
the value of any button or axis. Like any other input source, your inputs will time out and expire for safety. 

This has not yet been implemented.

## Control Modes

TODO: Write

### Using inputs directly in ROS2 *(for slop lovers)*

Not all control systems benefit from writing `ControlMode` plugins. Maybe you just want to write slop, and use 
those input values directly, as is. I won't judge you for it. I want to empower you to write slop. 

However, I want your slop to benefit from being able to easily support multiple input sources through configuration. You
should use big `teleop` too.

So, `teleop` will be able to publish all of it's input on each update through a topic. 

```yaml
teleop:
  ros__parameters:
    # Configure the node to always publish some specified inputs 
    publish:
      axes: [
        "auger"
        "platform"
      ]
      buttons: [
        "locked"
      ]
      events: []
```

This is not yet implemented.


