"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the rover to start the Unity
    visualisations and the back-end script.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - visualisation/arm_poses_sub.py       [arm_poses]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        core
CREATION:       14/05/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

# Include the required launch parameters
from launch import LaunchDescription
import launch_ros.actions

# Generate the launch file with all inputs
def generate_launch_description():
    return LaunchDescription([
        launch_ros.actions.Node(
            package='visualisation', node_executable='arm_poses_sub.py', output='screen')
    ])
