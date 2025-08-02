# teleop\_drive\_joy

\================

## Overview

The purpose of this package is to provide a flexible facility for tele-operating the rover using game controllers. It supports Twist-based and DriveInput-based control modes, translating joystick inputs into velocity commands or custom drive inputs.

This package relies on the [joy](https://index.ros.org/p/joy/github-ros-drivers-joystick_drivers/#foxy) driver for reading joystick inputs. It does not implement rate limiting or autorepeat functionality, as these are available through the `joy` driver.

## Controller Layout
![image](https://github.com/user-attachments/assets/e11ba273-9978-4f00-82f8-a08b0a74214a)

## Executables

The package includes the `teleop_drive_joy_node`, which translates `sensor_msgs/msg/Joy` messages into:

- `geometry_msgs/msg/Twist` messages for velocity commands (For use in Autonomous).
- `nova_interfaces/msg/DriveInputStamped` messages for drive inputs.

## Subscribed Topics

- `/joy (sensor_msgs/msg/Joy)`
  - Joystick messages to be translated into commands.

## Published Topics

- `/cmd_vel (geometry_msgs/msg/TwistStamped)`
  - Velocity commands derived from joystick input.
- `/drive_input (nova_interfaces/msg/DriveInputStamped)`
  - Drive input messages for custom rover control.
- `/drive_info (nova_interfaces/msg/DriveInfo)`
  - Drive state information.

## Parameters

### General Parameters

- `joystick (string, default: 'xbox')`
  - The type of joystick configuration file to load.
- `joy_dev (string, default: '/dev/input/js0')`
  - Path to the joystick device.

### Control Parameters

- `require_enable_button (bool, default: true)`
  - Whether an enable button must be pressed to allow movement.
- `enable_button (int, default: 0)`
  - Joystick button to enable movement.

### Axis Mappings

- `axis_linear_<axis>`

  - Specifies which joystick axis controls linear movement.
  - `axis_linear_x (int)`
  - `axis_linear_y (int)`
  - `axis_linear_z (int)`

- `axis_angular_<axis>`

  - Specifies which joystick axis controls angular movement.
  - `axis_angular_x (int)`
  - `axis_angular_y (int)`
  - `axis_angular_z (int)`

### Scaling Parameters

- `scale_linear_<axis>`

  - Scale factor for regular-speed linear movement.
  - `scale_linear_x (double)`
  - `scale_linear_y (double)`
  - `scale_linear_z (double)`

- `scale_angular_<axis>`

  - Scale factor for regular-speed angular movement.
  - `scale_angular_x (double)`
  - `scale_angular_y (double)`
  - `scale_angular_z (double)`


## Usage

### Running the Node

A launch file is provided for convenience. To run the node with default settings:

```bash
ros2 launch teleop_drive_joy teleop.launch.py
```

### Custom Configuration

To use a specific joystick configuration (e.g., `ps3` or `xbox`):

```bash
ros2 launch teleop_drive_joy teleop.launch.py joystick:=xbox
```

The package includes sample configuration files for common controllers (e.g., `ps3`, `xbox`) located in the `config` directory.

#### Example Configuration File: Xbox Series X / S Controller

Below is an example configuration for the Xbox Series X / S controller. This file specifies mappings and parameters for controlling the rover:

```yaml
# Vanilla XBox Series X / S Controller
teleop_drive_joy_node:
  ros__parameters:
    controllers:
      [
        "pivot_drive_controller",
        "strafe_controller",
        "diff_drive_controller",
      ]
    pivot_drive_controller:
      axis_angular_z: 3
      axis_linear_x: 1
      scale_angular_z: 1.0
      scale_linear_x: 0.1
    strafe_controller:
      axis_angular_z: 3
      axis_linear_x: 0
      scale_angular_z: 1.0
      scale_linear_x: 0.1
    diff_drive_controller:
      axis_angular_z: 3
      axis_linear_x: 1
      scale_angular_z: 1.0
      scale_linear_x: 0.1
    axis_speed_change_coarse: 7
    axis_speed_change_fine: 6
    button_autonomous_control: 0
    button_lock: 6
    button_manual_control: 1
    button_diff_drive_controller: 3
    button_pivot_drive_controller: 5
    button_strafe_controller: 4
    button_unlock: 7
    speed_change_coarse_val: 0.1
    speed_change_fine_val: 0.02
    speed_limit_max: 1.2
    speed_limit_min: 0.05
```

### Launch Arguments

- `joystick (string, default: 'xbox')`
  - Specifies the joystick configuration.
- `joy_dev (string, default: '/dev/input/js0')`
  - Specifies the joystick device path.

### Notes

- The `joy` node is launched automatically by the provided launch file. Do not launch it separately.
- Adding a new controller can be done by observing joy inputs using `ros2 topic echo /joy` and mapping controls to desired control parameters.

