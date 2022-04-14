"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the rover to start all
   arm control scripts.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - control/arm/arm_inputs              [arm_inputs]
  - control/arm/arm_kinematics          [arm_kinematics]
  - control/arm/arm_driver              [arm_driver]
  - electronics/electronics             [resolver_publisher.py]
  - visualisation/arm_viz_publisher     [arm_viz_publisher]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	core
CREATION:	17/12/2021
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
            package='control', node_executable='arm_driver', output='screen'),
        launch_ros.actions.Node(
            package='control', node_executable='arm_kinematics', output='screen'),
        launch_ros.actions.Node(
            package='electronics', node_executable='resolver_publisher.py', output='screen'),
        launch_ros.actions.Node(
            package='visualisation', node_executable='arm_viz_publisher', output='screen'),
    ])
