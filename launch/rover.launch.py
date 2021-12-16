"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the rover to start all
   rover control scripts.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - control/inputs/drive_publisher     [drive_cmd]
  - control/inputs/drive_subscriber    [driver]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
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
    ])