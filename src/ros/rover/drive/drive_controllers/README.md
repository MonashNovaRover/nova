# drive_controllers

This is where all our drive controllers reside.

## Configuration

Please configure parameters in the relevant `controllers.yaml` file. For auto, this is `auto/auto_bringup/params/controllers.yaml`.

## Parameters (Common)
<details>

These are parameters every controller has.

`autonomous_mode` (`bool`, default: `false`)
- Changes how the controller reacts to Twist commands

`input_curve_factor` (`double`, default: `2.0`)
- Turning radius control curve factor for manual control. Higher values = steeper curves, steeper curves = finer control over higher turning radii (slight turns)

`left_drive_names` (`string_array`, default: `[]`)
- Link names of the left side wheels

`right_drive_names` (`string_array`, default: `[]`)
- Link names of the right side wheels

`left_pivot_names` (`string_array`, default: `[]`)
- Link names of the left side pivots

`right_pivot_names` (`string_array`, default: `[]`)
- Link names of the right side pivots

`steering_track` (`double`, default: `0.81564001`)
- Distance between center of left and right wheels

`wheel_base` (`double`, default: `0.95752883`)
- Distance between center of front and rear wheels

`steering_track` (`double`, default: `0.15692321`)
- Radius of wheels

`tf_frame_prefix_enable` (`bool`, default: `true`)
- Enables or disables appending `tf_frame_prefix` to tf frame ids

`tf_frame_prefix` (`string`, default: `""`)
- (optional) Prefix to be appended to the tf frames, will be added to `odom_id` and `base_frame_id` before publishing. If the parameter is empty, controller's namespace will be used

`odom_frame_id` (`string`, default: `odom`)
- Name of the frame for odometry. This frame is parent of `base_frame_id` when controller publishes odometry

`base_frame_id` (`string`, default: `base_link`)
- Name of the robot's base frame that is child of the odometry frame

`pose_covariance_diagonal` (`double_array`, default: `[0.0, 0.0, 0.0, 0.0, 0.0, 0.0]`)
- Odometry covariance for the encoder output of the robot for the pose. These values should be tuned to your robot's sample odometry data, but these values are a good place to start: `[0.001, 0.001, 0.001, 0.001, 0.001, 0.01]`

`twist_covariance_diagonal` (`double_array`, default: `[0.0, 0.0, 0.0, 0.0, 0.0, 0.0]`)
- Odometry covariance for the encoder output of the robot for the speed. These values should be tuned to your robot's sample odometry data, but these values are a good place to start: `[0.001, 0.001, 0.001, 0.001, 0.001, 0.01]`

`open_loop` (`bool`, default: `false`)
- If set to true the odometry of the robot will be calculated from the commanded values and not from feedback

`drive_position_feedback` (`bool`, default: `false`)
- Whether the drives have position or velocity feedback

`pivot_position_feedback` (`bool`, default: `true`)
- Whether the pivots have position or velocity feedback

`enable_odom_tf` (`bool`, default: `true`)
- Publish transformation between `odom_frame_id` and `base_frame_id`

`cmd_vel_timeout` (`double`, default: `0.5`)
- Timeout on cmd_vel command

`publish_commanded_velocities` (`bool`, default: `false`)
- Publish the commanded velocities as a TwistStamped message

`velocity_rolling_window_size` (`int`, default: `10`)
- Size of the rolling window for calculation of mean velocity use in odometry

`publish_rate` (`double`, default: `50.0`)
- Publishing rate (Hz) of the odometry and TF messages

`drive`/`angular`/`pivot`
- `has_velocity_limits` (`bool`, default: `false`)
- `has_acceleration_limits` (`bool`, default: `false`)
- `has_jerk_limits` (`bool`, default: `false`)
- `max_velocity` (`double`, default: `.NAN`)
- `min_velocity` (`double`, default: `.NAN`)
- `max_acceleration` (`double`, default: `.NAN`)
- `min_acceleration` (`double`, default: `.NAN`)
- `max_jerk` (`double`, default: `.NAN`)
- `min_jerk` (`double`, default: `.NAN`)
</details>

## Parameters (Controller-Specific)

<details>

These are parameters specific to certain controllers.

### pivot_drive_controller
`pivot_rate_tolerance` (`double`, default: `0.1`)
- In autonomous mode, we may want to delay movement until pivots have reached the desired angle. This parameter controls **how long in seconds** before the pivots reach their target positions to start moving. Only affects autonomous mode operation

</details>