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
from ament_index_python.packages import get_package_share_path

# Generate the launch file with all inputs
def generate_launch_description():
    # Get the path to the parameters file in core/params
    core_params_path = get_package_share_path('core') / "params"
    
    return LaunchDescription([      
        launch_ros.actions.Node(
            package='control', node_executable='arm_inputs', output='screen', emulate_tty=True),
        launch_ros.actions.Node(
            package='control', node_executable='arm_twistmapper', output='screen', emulate_tty=True),
        launch_ros.actions.Node(
            package='control', node_executable='arm_control', output='screen', emulate_tty=True),
        launch_ros.actions.Node(
            package='control', node_executable='arm_driver', output='screen', emulate_tty=True),
        launch_ros.actions.Node(
            package='electronics', node_executable='resolver_publisher.py', parameters=[core_params_path / 'arm_params.yaml'], output='screen', emulate_tty=True),
        launch_ros.actions.Node(
            package='control', node_executable='arm_rviz_publisher', output='screen', emulate_tty=True),
    ])
