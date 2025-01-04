'''
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
'''
from launch import LaunchDescription
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch.conditions import UnlessCondition, IfCondition
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction, GroupAction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    nova_bringup_dir = FindPackageShare('nova_bringup')
    teleop_drive_joy_dir = FindPackageShare('teleop_drive_joy')
    
    arm_urdf = LaunchConfiguration('arm_urdf')
    arm_urdf_path = LaunchConfiguration('arm_urdf_path')
    rover_urdf = LaunchConfiguration('rover_urdf')
    teleop = LaunchConfiguration('teleop')
    urdf = LaunchConfiguration('urdf')

    return [
        GroupAction(
            condition=UnlessCondition(teleop),
            actions=[
                Node(
                    package='inputs',
                    executable='inputs_publisher',
                    output='screen',
                    emulate_tty=True,
                    parameters=[{'use_sim_time': False}],
                ),
                IncludeLaunchDescription(
                    condition=IfCondition(urdf),
                    launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([nova_bringup_dir, 'launch', 'urdf.launch.py'])),
                    launch_arguments = {
                        'arm_urdf_path': arm_urdf_path,
                        'arm': arm_urdf,
                        'rover': rover_urdf,
                    }.items(),
                ),
            ],
        ),
        IncludeLaunchDescription(
            condition=IfCondition(teleop),
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([teleop_drive_joy_dir, 'launch', 'teleop.launch.py'])),
        ),
    ]

def generate_launch_description():
    rover_description_dir = FindPackageShare('rover_description')

    declared_arguments = [      
        DeclareLaunchArgument(
            name='arm_urdf', 
            default_value='False',
            description='Include arm URDF in robot_description?',
        ),
        DeclareLaunchArgument(
            name='arm_urdf_path', 
            default_value = PathJoinSubstitution([rover_description_dir, 'arm', 'urdf', 'arm.urdf.xacro']), 
        ),
        DeclareLaunchArgument(
            name='rover_urdf', 
            default_value='True',
            description='Include rover URDF in robot_description?',
        ),
        DeclareLaunchArgument(
            name='teleop', 
            default_value='False',
            description='Use ROS2 Controllers?',
        ),
        DeclareLaunchArgument(
            name='urdf', 
            default_value='False',
            description='Publish robot_description?',
        ),
    ]

    return LaunchDescription(  
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )