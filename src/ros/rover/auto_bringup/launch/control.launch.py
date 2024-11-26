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
from ament_index_python.packages import get_package_share_directory

# Include the required launch parameters
from launch import LaunchDescription
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch.conditions import UnlessCondition
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from launch_ros.descriptions import ParameterValue


def launch_setup(context, *args, **kwargs):
    auto_bringup_dir = FindPackageShare('auto_bringup')
    
    controllers = LaunchConfiguration('controllers')
    gazebo = LaunchConfiguration('gazebo')
    model = LaunchConfiguration('model')

    return [
        Node(
            package='controller_manager',
            executable='ros2_control_node',
            parameters=[controllers],
            remappings=[('/controller_manager/robot_description', '/robot_description')],
            condition = UnlessCondition(gazebo)
        ),
        # Node(
        #     package='controller_manager',
        #     executable='spawner',
        #     arguments=['wheel_velocity_controller']
        # ),

        # Node(
        #     package='controller_manager',
        #     executable='spawner',
        #     arguments=['pivot_joint_trajectory_controller']
        # ),
        # Node(
        #     package='controller_manager',
        #     executable='spawner',
        #     arguments=['strafe_controller', '--inactive']
        # ),
        # Node(
        #     package='controller_manager',
        #     executable='spawner',
        #     arguments=['nova_diff_drive_controller', '--inactive']
        # ),
        Node(
            package='controller_manager',
            executable='spawner',
            arguments=['pivot_drive_controller']
        ),
        Node(
            package='controller_manager',
            executable='spawner',
            arguments=['joint_broad']
        ),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'urdf.launch.py'])),
            condition=UnlessCondition(gazebo),
            launch_arguments={'model': model, 'gazebo': 'false'}.items()
        ),
    ]


def generate_launch_description():
    auto_bringup_dir = FindPackageShare('auto_bringup')
    rover_description_dir = FindPackageShare('rover_description')

    declared_arguments = [      
        DeclareLaunchArgument(
            name='controllers',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'controllers.yaml']),
            description='Path of the controller params file',
        ),
        DeclareLaunchArgument(
            name='gazebo',
            default_value='False',
            description='Use simulation (Gazebo) clock if true',
        ),
        DeclareLaunchArgument(
            name='model', 
            default_value=PathJoinSubstitution([rover_description_dir, 'urdf', 'rover.urdf.xacro']),
            description='Absolute path to robot urdf file',
        ),  
    ]

    return LaunchDescription(  
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
