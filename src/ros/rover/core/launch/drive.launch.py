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
            package='control', executable='drive_inputs', output='screen', emulate_tty=True),
        launch_ros.actions.Node(
            package='control', executable='driver', output='screen', emulate_tty=True),
        # launch_ros.actions.Node(
        #     package='electronics', executable='wheel_publisher.py', output='screen', emulate_tty=True),
        # launch_ros.actions.Node(
	    #     package='electronics', executable='gimbal_service.py', output='screen', emulate_tty=True),
        # launch_ros.actions.Node(
	    #     package='electronics', executable='LED_transmitter.py', output='screen', emulate_tty=True),
        # launch_ros.actions.Node(
        #     package='imu',  executable='imu_node', output='screen', emulate_tty=True),
        # launch_ros.actions.Node(
        #     package='electronics', executable='CMD_service.py', output='screen', emulate_tty=True), 
        
    ])
