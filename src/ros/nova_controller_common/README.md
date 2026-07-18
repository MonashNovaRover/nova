# nova_controller_common
This package contains common utils that can be used across any ros2_control controller.

## Included Utils:
- `SpeedLimiter` - taken directly from the Humble version of ros2_control's `diff_drive_controller`. Limits speed based on velocity, acceleration and jerk.
- `PositionLimiter` - based on SpeedLimiter, limits position based on velocity, acceleration and jerk.
- `HardwareInterfaceWrapper` - a wrapper around hardware interfaces to provide cleaner access to joint handles.