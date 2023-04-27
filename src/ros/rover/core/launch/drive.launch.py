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
from launch.conditions import IfCondition, UnlessCondition
from launch.actions import IncludeLaunchDescription, DeclareLaunchArgument
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from ament_index_python.packages import get_package_share_path


# Generate the launch file with all inputs
def generate_launch_description():
    urdf_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            FindPackageShare("core"), '/launch', '/urdf.launch.py'
        ])
    )
    return LaunchDescription([
        urdf_launch,
        Node(
            package='control', executable='drive_inputs', output='screen', emulate_tty=True),
        Node(
            package='control', executable='driver', output='screen', emulate_tty=True),
        Node(
	        package='electronics', executable='LED_transmitter.py', output='screen', emulate_tty=True),
        Node(
            package='imu',  executable='imu_node', output='screen', emulate_tty=True),
        # launch_ros.actions.Node(
	    #     package='electronics', executable='gimbal_service.py', output='screen', emulate_tty=True),
    ])
