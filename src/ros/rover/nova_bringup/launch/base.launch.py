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
#
# Include the required launch parameters
from launch import LaunchDescription
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch_ros.actions import Node
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.conditions import IfCondition

# Generate the launch file with all inputs
def generate_launch_description():
    description_dir = FindPackageShare('rover_description')
    gazebo = LaunchConfiguration('gazebo', default=False)

    return LaunchDescription([
        DeclareLaunchArgument(
            'arm_urdf', 
            default_value='True',
            description="Include arm URDF in robot_description?"
        ),

        DeclareLaunchArgument(
            'rover_urdf', 
            default_value='True',
            description="Include rover URDF in robot_description?"
        ),
                              
        DeclareLaunchArgument(
            'urdf', 
            default_value='False',
            description="Publish robot_description?"
        ),

        DeclareLaunchArgument(
            'arm_urdf_path', 
            default_value = PathJoinSubstitution(
                [
                    description_dir, 
                    'urdf', 
                    'arm.urdf.xacro'
                ]
            ), 
        ),

        Node(
            package='inputs',
            executable='inputs_publisher',
            output='screen',
            emulate_tty=True,
            parameters=[{'use_sim_time': gazebo}]
        ),

        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                PathJoinSubstitution(
                    [
                        FindPackageShare('nova_bringup'),
                        'launch',
                        'urdf.launch.py'
                    ]
                )
            ),
            launch_arguments = {
                "arm_urdf_path": LaunchConfiguration('arm_urdf_path'),
                "arm": LaunchConfiguration('arm_urdf'),
                "rover": LaunchConfiguration('rover_urdf')
            }.items(),
            condition=IfCondition(LaunchConfiguration('urdf'))
        ),
    ])
