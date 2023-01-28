# How to do autonomous in comp for URC

## On mac:
### Mac Computer:
`base` \
`ros2 run cameras cient chassis` \

### Mac Jetson:
`ros2 run cameras service` \

## On Metabox:

### Metabox Computer
`cd ~/nova_ws/src/rover/autonomous/autonomous/` \
`rviz2 -d config/auto.rviz` \
`python3 vis/rover_vis.py` \
`python3 vis/path_vis.py` \
`python3 update_goals.py` \

### Metabox Jetson
`can start all` \
`rover` \
`dgps` \
`cd ~/nova_ws/src/rover/autonomous/autonomous/` \
`python3 localisation/pose_converter.py` \
`python3 main.py`
