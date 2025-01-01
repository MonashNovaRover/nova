'''
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
CREATION:   17/12/2021
EDITED:     01/01/2025
EDITED BY: Taaj Street, Dylan Gonzalez, Tristan 
    Clark, Matthew Gu, Victor Bartlinski
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction, GroupAction
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch.conditions import IfCondition, UnlessCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def launch_setup(context, *args, **kwargs):
    nova_bringup_dir = FindPackageShare('nova_bringup')

    sim = LaunchConfiguration('sim')
    urdf = LaunchConfiguration('urdf')
    namespace = LaunchConfiguration('namespace')
    chassis_cam = LaunchConfiguration('chassis_cam')
    params = LaunchConfiguration('params')

    return [
        Node(
            package='arm', 
            executable='arm_rviz_publisher', 
            namespace=namespace,
            output='screen', 
            emulate_tty=True,
        ),
        Node(
            package='arm', 
            executable='arm_control', 
            namespace=namespace,
            output='screen', 
            emulate_tty=True,
        ),
        Node(
            package='arm', 
            executable='arm_inputs', 
            namespace=namespace,
            output='screen', 
            emulate_tty=True,
        ),
        Node(
            package='arm', 
            executable='arm_twistmapper', 
            namespace=namespace,
            output='screen', 
            emulate_tty=True,
        ),
        Node(
            package='arm', 
            executable='hex_key.py', 
            output='screen', 
            emulate_tty=True,
        ),
        Node(
            package='arm',
            executable='lazers.py',
            output='screen',
            emulate_tty=True
        ),
        Node(
            package='gimbal_cam', 
            executable='gimbal_cam',
            parameters=[{'chassis_cam': chassis_cam}],
            output='screen', 
            emulate_tty=True,
        ),
        Node(
            package='arm', 
            executable='resolver_spoofer', 
            namespace=namespace,
            output='screen', 
            condition=IfCondition(sim), 
            emulate_tty=True,
        ),
        IncludeLaunchDescription(
            condition=IfCondition(urdf),
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([nova_bringup_dir, 'launch', 'urdf.launch.py'])),
        ),
        GroupAction(
            condition=UnlessCondition(sim),
            actions=[
                Node(
                    package='arm', 
                    executable='arm_driver', 
                    namespace=namespace,
                    output='screen', 
                    emulate_tty=True, 
                ),
                Node(
                    package='arm', 
                    executable='resolver_publisher.py', 
                    namespace=namespace,
                    parameters=[params],
                    output='screen', 
                    emulate_tty=True,
                ),
                Node(
                    package='cmd_utils', 
                    executable='CMD_publisher.py', 
                    namespace=namespace,
                    output='screen', 
                    emulate_tty=True,
            )],
        ),
    ]

def generate_launch_description():
    nova_bringup_dir = FindPackageShare('nova_bringup')
    rover_description_dir = FindPackageShare('rover_description')

    declared_arguments = [
        DeclareLaunchArgument(
            name='sim', 
            default_value='False',
        ),
        DeclareLaunchArgument(
            name='params', 
            default_value=PathJoinSubstitution([nova_bringup_dir, 'params', 'arm_params.yaml']),
        ),
        DeclareLaunchArgument(
            name='arm', 
            default_value='True',
            description='Include arm URDF in robot_description?',
        ),
        DeclareLaunchArgument(
            name='rover_urdf', 
            default_value='True',
            description='Include rover URDF in robot_description?',
        ),
        DeclareLaunchArgument(
            name='urdf', 
            default_value='False',
            description='Publish robot_description?',
        ),
        DeclareLaunchArgument(
            name='arm_urdf_path', 
            default_value = PathJoinSubstitution([rover_description_dir, 'arm', 'urdf', 'arm.urdf.xacro']), 
        ),
        DeclareLaunchArgument(
            name='namespace', 
            default_value = '',
        ),
        DeclareLaunchArgument(
            name='chassis_cam',
            default_value = 'False',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )