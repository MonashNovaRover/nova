# About
`auto_bringup` is a ROS2 [bringup package](https://roboticsbackend.com/package-organization-for-a-ros-stack-best-practices/#Bringup_package) for the Monash Nova Rover ROS2 autonomous system, containing all the necessary launch files and related parameters to execute and run the autonomous stack.

# Structure
### launch
* `camera`
  * `rtabmap_point_cloud_xyz`
  * `depth_filter`
  * `imu_transformer`
* `control`
  * `control_node`
  * `wheel_velocity_controller`
  * `pivot_joint_trajectory_controller`
  * `strafe_drive_controller`
  * `diff_drive_controller`
  * `pivot_drive_controller`
  * `joint_broad`
* `everything`
  * Super launch file that intends to run the whole autonomous stack with a single executable, by executing other launch files according to the specified parameters provided.
* `gazebo`
  * `spawn_rover_cmd`
* `localization`
  * `robot_localisation_node`
  * `gps_localisation_odom`
  * `gps_localisation_map`
  * `navsat_transform_node`
  * `slam_cmd`
  * `static_transform_node`
* `navigation`
  * `controller_server`
  * `smoother_server`
  * `planner_server`
  * `behavior_server`
  * `waypoint_follower`
  * `velocity_smoother`
  * `lifecycle_manager_navigation`
  * `bt_navigator`
  * `map_server`
* `rtabmap`
  * `rgbd_odometry`
  * `rtabmap`
  * `rtabmap_viz`
* `rviz`
  * `rviz_node`
* `urdf`
  * `robot_state_publisher_node`
### maps
* `static_map_layer`
  * Parameters for the static map layer.
  * (should be moved to params, obviously check this won't break anything beforehand)
### params
* `aruco_tracker`
  * Parameters for the `aruco_tracker` node from the `aruco_opencv` ROS2 package, used for AR tag detection.
* `auto_params`
* `controllers`
* `depthai_oakd`
* `depthai_oakd_rgbd`
* `ekf`
* `nav2`
* `pcl`
* `rl`
* `ukf`
### resources
### rviz
* `navigation`
  * Defines interface settings for rviz, to use when launched during navigation.
