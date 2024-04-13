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
from launch.substitutions import LaunchConfiguration
from launch.conditions import UnlessCondition
from launch_ros.actions import Node


# Generate the launch file with all inputs
def generate_launch_description():
    gazebo = LaunchConfiguration('gazebo', default=False)
    return LaunchDescription([
        Node(
            package='drive', executable='drive_inputs', output='screen', emulate_tty=True,
            parameters=[{'use_sim_time': gazebo}]),
        Node(
            package='drive', executable='driver', output='screen', emulate_tty=True,
            parameters=[{'use_sim_time': gazebo, 'gazebo': gazebo}]),
        Node(
            package='blcmd_utils', executable='blcmd_status_monitor.py', output='screen', emulate_tty=True,
        ),
        # Node(
        #     package='electronics', executable='LED_transmitter.py', output='screen', emulate_tty=True,
        #     condition=UnlessCondition(gazebo)),
    ])
