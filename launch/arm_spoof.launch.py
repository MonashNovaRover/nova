"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the rover to start all
   control scripts for simulated operation of the arm.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - control/arm/arm_inputs              [arm_inputs]
  - control/arm/arm_kinematics          [arm_kinematics]
  - control/arm/resolver_spoofer        [resolver_spoofer]
  - visualisation/arm_viz_publisher     [arm_viz_publisher]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	core
CREATION:	1/03/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

# Include the required launch parameters
from launch import LaunchDescription
import launch_ros.actions

# Generate the launch file with all inputs
def generate_launch_description():
    return LaunchDescription([      
        launch_ros.actions.Node(
            package='control', node_executable='arm_inputs', output='screen'),
        launch_ros.actions.Node(
            package='control', node_executable='arm_kinematics', output='screen'),
        launch_ros.actions.Node(
            package='control', node_executable='resolver_spoofer', output='screen'),
        launch_ros.actions.Node(
            package='visualisation', node_executable='arm_viz_publisher', output='screen'),
    ])
