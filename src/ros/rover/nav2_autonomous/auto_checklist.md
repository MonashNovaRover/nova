## ARCH 2024 Checklist

### On TX2
`nvidia@10.0.0.10`

`ros2 launch core control_launch.py gazebo:=True` on TX2: `nvidia@10.0.0.10`

### On Orin
`nova@10.0.0.11`

```
rm ~/.ros/rtabmap.db
ros2 launch core camera.launch.py
```
```
ros2 launch core localization_launch.py
```
```
ros2 launch core navigation_launch.py
```

```
ros2 run nova_cube_localisation image_capture.py
```

```
ros2 run nova_cube_localisation image_capture.py
```

```
ros2 run image_view image_saver --ros-args --remap --param filename_format:='rgb%04i.png
```

### On Base Station

```
ros2 run image_view image_saver --ros-args --remap --param filename_format:='rgb%04i.png
```

```
rviz2 -d src/ros/rover/core/rviz/rover.rviz
```
