# teleop\_drive\_joy

The purpose of this package is to provide a flexible facility for tele-operating the rover using game controllers.

This package relies on the [joy](https://index.ros.org/p/joy/github-ros-drivers-joystick_drivers/#foxy) driver for reading joystick inputs. It does not implement rate limiting or autorepeat functionality, as these are available through the `joy` driver.

## Controller Layout
<img width="1177" height="649" alt="image" src="https://github.com/user-attachments/assets/cd1e35ca-618e-428e-a303-358546d8a6f7" />

Controller layout diagram made with [PadCrafter](https://www.padcrafter.com/).

## Overview

The package includes the `teleop_drive_joy_node`, which translates `sensor_msgs/msg/Joy` messages into `geometry_msgs/msg/Twist` messages comprised of linear and angular velocity.
Controllers will interpret the Twist message differently depending on whether or not they are in autonomous mode. In autonomous mode, Twist messages are used directly, i.e.
linear and angular velocity are used directly. In manual mode, the angular velocity is often converted into a turning radius through a curve.

## Usage

### Running the Node

A launch file is provided for convenience. To run the node with default settings:

```bash
ros2 launch teleop_drive_joy teleop.launch.py
```

The connected game controller should be automatically recognised, with correctly mapped inputs.

### Launch Arguments

- `device_id (int, default: 0)`
  - Specifies the joystick device id.
- `device_name (string, default: '')`
  - Specifies the joystick name. This can be useful when multiple different joysticks are attached. If both device_name and device_id are specified, device_name takes precedence.
- `joy_vel (string, default: 'cmd_vel')`
  - Specifies the topic to remap /cmd_vel to.
