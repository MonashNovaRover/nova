'''
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
CREATION:	29/12/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from launch import LaunchDescription
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch.conditions import UnlessCondition
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction, GroupAction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def launch_setup(context, *args, **kwargs):
    nova_bringup_dir = FindPackageShare('nova_bringup')
    
    controllers = LaunchConfiguration('controllers')
    gazebo = LaunchConfiguration('gazebo')
    model = LaunchConfiguration('model')

    return [
        Node(
            package='controller_manager',
            executable='spawner',
            arguments=['pivot_drive_controller', '--switch-timeout', '10'] #, '--inactive']
        ),
        GroupAction(
            condition=UnlessCondition(gazebo),
            actions=[
                Node(
                    package='controller_manager',
                    executable='spawner',
                    arguments=['joint_state_broadcaster']
                ),
                Node(
                    package='controller_manager',
                    executable='ros2_control_node',
                    parameters=[controllers],
                    remappings=[('/controller_manager/robot_description', '/robot_description')],
                ),
                IncludeLaunchDescription(
                    PythonLaunchDescriptionSource(PathJoinSubstitution([nova_bringup_dir, 'launch', 'urdf.launch.py'])),
                    launch_arguments={'model': model, 'gazebo': gazebo}.items(),
                )],
        ),
    ]


def generate_launch_description():
    nova_bringup_dir = FindPackageShare('nova_bringup')
    rover_description_dir = FindPackageShare('rover_description')

    declared_arguments = [      
        DeclareLaunchArgument(
            name='controllers',
            default_value=PathJoinSubstitution([nova_bringup_dir, 'params', 'controllers.yaml']),
            description='Absolute path to controller params file',
        ),
        DeclareLaunchArgument(
            name='gazebo',
            default_value='False',
            description='Use simulation (Gazebo) clock if True',
        ),
        DeclareLaunchArgument(
            name='model', 
            default_value=PathJoinSubstitution([rover_description_dir, 'base', 'urdf', 'auger_ros2_control.xacro']),
            description='Absolute path to robot urdf file',
        ),  
    ]

    return LaunchDescription(  
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
