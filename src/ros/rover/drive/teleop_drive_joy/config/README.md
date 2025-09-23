# Teleop Drive Joy Parameters

Default Config

```yaml
teleop_drive_joy_node:
  ros__parameters:
    controllers:
    [
      "pivot_drive_controller",
      "strafe_drive_controller",
      "diff_drive_controller",
    ]
    <controllers>:
      axis_angular_z: 3
      axis_linear_x: 1
      scale_angular_z: 1.0
      scale_linear_x: 0.1
    axis_speed_change_coarse: 7
    axis_speed_change_fine: 6
    button_autonomous_control: 0
    button_lock: 10
    button_manual_control: 1
    button_diff_drive_controller: 4
    button_pivot_drive_controller: 7
    button_strafe_drive_controller: 6
    button_unlock: 9
    speed_change_coarse_val: 0.1
    speed_change_fine_val: 0.02
    speed_limit_max: 1.2
    speed_limit_min: 0.05
```

## controllers

List Of Controllers that can be used with teleop

- Type: `string_array`
- Default Value: [
  "pivot_drive_controller",
  "strafe_drive_controller",
  "diff_drive_controller",
  ]
- Read only: True

## button_unlock

Unlock Gamepad

- Type: `int`
- Default Value: 9

## button_lock

Lock Gamepad

- Type: `int`
- Default Value: 10

## button_strafe_drive_controller

Switch to Strafe Mode

- Type: `int`
- Default Value: 6

## button_pivot_drive_controller

Switch to Pivot Mode

- Type: `int`
- Default Value: 7

## button_diff_drive_controller

Switch to Tank Mode

- Type: `int`
- Default Value: 4

## button_autonomous_control

Enable autonomous operation - set enable_twist_cmd true for pivot_drive_controller

- Type: `int`
- Default Value: 0

## button_manual_control

Enable manual operation - set enable_twist_cmd false for pivot_drive_controller

- Type: `int`
- Default Value: 1

## axis_speed_change_coarse

Button to increment current speed scaling.

- Type: `int`
- Default Value: 7

## axis_speed_change_fine

Button to increment current speed scaling.

- Type: `int`
- Default Value: 6

## speed_change_coarse_val

Value to change speed when the speed_change_coarse axis is pressed

- Type: `double`
- Default Value: 0.1

## speed_change_fine_val

Value to change speed when the speed_change_fine axis is pressed

- Type: `double`
- Default Value: 0.02

## <controllers>.axis_linear_x

The Axis controlling linear velocity. Usually refers to the Left Stick Verical Axis

- Type: `int`
- Default Value: 1

## <controllers>.scale_linear_x

A scale parameter applied on linear velocity

- Type: `double`
- Default Value: 0.1

## <controllers>.axis_angular_z

The Axis controlling angular velocity. Usually refers to the Right Stick Horizontal Axis

- Type: `int`
- Default Value: 3

## <controllers>.scale_angular_z

A scale parameter applied on angular velocity

- Type: `double`
- Default Value: 1.0

## speed_limit_max

Max speed that gets the Rover Arrested or the Max Clipping Value of Linear Velocity

- Type: `double`
- Default Value: 1.2

## speed_limit_min

Min speed that gets the Rover get Honked At or the Min Clipping Value of Linear Velocity

- Type: `double`
- Default Value: 0.05
