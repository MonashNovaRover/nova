"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the base station to start all
    base station scripts.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - control/inputs/inputs_publisher     [inputs]
  - electronics/electronics/radio_monitor.py
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	core
CREATION:	15/12/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

# Include the required launch parameters
from launch import LaunchDescription
from launch.substitutions import LaunchConfiguration
from launch.actions import IncludeLaunchDescription, DeclareLaunchArgument
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import PathJoinSubstitution, TextSubstitution

from launch_ros.substitutions import FindPackageShare
from launch_ros.actions import Node


# Generate the launch file with all inputs
def generate_launch_description():

    return LaunchDescription([
       DeclareLaunchArgument('rfid_port', default_value='/dev/ttyUSB0'),
       IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            PathJoinSubstitution([
                FindPackageShare('core'),
                'launch',
                'drive.launch.py',
            ]
            )
        )),
       IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            PathJoinSubstitution([
                FindPackageShare('core'),
                'launch',
                'arm.launch.py'
            ]
            )
        )),
        Node(
            package='electronics',
            executable='rfid_service.py',
            name='rfid_node',
            parameters=[{'port': LaunchConfiguration('rfid_port')}]
        )
        
    ])