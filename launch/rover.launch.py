"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the rover to start all
   rover control scripts.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - control/drive/drive_inputs      [drive_cmd]
  - control/drive/driver            [driver]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	core
CREATION:	15/12/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

# Include the required launch parameters
from launch import LaunchDescription
import launch_ros.actions

# Generate the launch file with all inputs
def generate_launch_description():
    return LaunchDescription([
        launch_ros.actions.Node(
            package='control', node_executable='drive_inputs', output='screen'),
        launch_ros.actions.Node(
            package='control', node_executable='driver', output='screen'),
        launch_ros.actions.Node(
            package='electronics', node_executable='wheel_publisher.py', output='screen'),
        launch_ros.actions.Node(
	        package='electronics', node_executable='gimbal_service.py', output='screen'),
        launch_ros.actions.Node(
	        package='electronics', node_executable='LED_transmitter.py', output='screen'),
        launch_ros.actions.Node(
            package='imu',  node_executable='imu_node', output='screen'),
    ])
