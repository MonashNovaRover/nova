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
CREATION:	15/12/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from launch import LaunchDescription
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch.actions import DeclareLaunchArgument, OpaqueFunction, GroupAction, IncludeLaunchDescription
from launch.conditions import UnlessCondition, IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    nova_bringup_dir = FindPackageShare('nova_bringup')

    arm_urdf = LaunchConfiguration('arm_urdf')
    arm_urdf_path = LaunchConfiguration('arm_urdf_path')
    controllers = LaunchConfiguration('controllers')
    rover_urdf = LaunchConfiguration('rover_urdf')
    teleop = LaunchConfiguration('teleop')

    return [
        GroupAction(
            condition=IfCondition(teleop),
            actions=[
                Node(
                    package='blcmd_utils', 
                    executable='status_monitor', 
                    output='screen', 
                    emulate_tty=True,
                ),
                Node(
                    package='controller_manager',
                    executable='spawner',
                    arguments=['pivot_drive_controller', '--switch-timeout', '10'],
                ),
                Node(
                    package='controller_manager',
                    executable='spawner',
                    arguments=['strafe_controller', '--inactive'],
                ),
                Node(
                    package='controller_manager',
                    executable='spawner',
                    arguments=['nova_diff_drive_controller', '--inactive'],
                ),
                Node(
                    package='controller_manager',
                    executable='spawner',
                    arguments=['joint_state_broadcaster'],
                ),
                Node(
                    package='controller_manager',
                    executable='ros2_control_node',
                    parameters=[controllers],
                    remappings=[('/controller_manager/robot_description', '/robot_description')],
                ),
                IncludeLaunchDescription(
                    launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([nova_bringup_dir, 'launch', 'urdf.launch.py'])),
                    launch_arguments = {
                        'arm_urdf_path': arm_urdf_path,
                        'arm': arm_urdf,
                        'rover': rover_urdf,
                    }.items(),
                ),
            ],
        ),
        GroupAction(
            condition=UnlessCondition(teleop),
            actions=[
                Node(
                    package='drive',
                    executable='drive_inputs',
                    output='screen',
                    emulate_tty=True,
                    parameters=[{'use_sim_time': False}],
                ),
                Node(
                    package='drive',
                    executable='driver',
                    output='screen',
                    emulate_tty=True,
                    parameters=[{'use_sim_time': False, 'gazebo': False}],
                ),
            ],
        ),
    ]


def generate_launch_description():
    nova_bringup_dir = FindPackageShare('nova_bringup')
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
            name='controllers',
            default_value=PathJoinSubstitution([nova_bringup_dir, 'params', 'controllers.yaml']),
            description='Absolute path to controller params file',
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
    ]

    return LaunchDescription(  
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )