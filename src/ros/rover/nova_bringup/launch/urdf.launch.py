'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the rover to publish the urdf
    static transforms and associated joint states
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 
NODES:
  - robot_state_publisher
  - rover_state_publisher
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CREATION:   13/05/2024
EDITED:     04/02/2025
EDITED BY:  Matthew Gu, Dylan Gonzalez, Jed Wong
    Taaj Street, Victor Bartlinski
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.substitutions import Command, LaunchConfiguration, PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch.conditions import IfCondition

def launch_setup(context, *args, **kwargs):
    nova_bringup_dir = FindPackageShare('nova_bringup')
    
    arm = LaunchConfiguration('arm').perform(context)
    arm_urdf_path = LaunchConfiguration('arm_urdf_path')
    rover = LaunchConfiguration('rover').perform(context).lower()
    rover_urdf_path = LaunchConfiguration('rover_urdf_path')
    rviz = LaunchConfiguration('rviz')

    if rover == 'true':
        fixed_frame = 'base_link'
        robot_description = ParameterValue(Command(['xacro ', rover_urdf_path, ' arm:=', arm, ' auto_camera:=false']), value_type=str)
    else:
        fixed_frame = 'arm_link'
        robot_description = ParameterValue(Command(['xacro ', arm_urdf_path, ' auto_camera:=false']), value_type=str)

    return [
        Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            parameters=[{'robot_description': robot_description}],
        ),
        Node(
            package='joint_state_publisher', 
            executable='joint_state_publisher', 
            namespace='',
            output='screen', 
            emulate_tty=True,
            parameters=[{'source_list': ['/arm/joint_states', '/joint_states']}],
        ),
        Node(
            package='rviz2',
            namespace='',
            executable='rviz2',
            name='rviz2',
            arguments=['-d', [PathJoinSubstitution([nova_bringup_dir, 'rviz', 'default.rviz'])], '-f', fixed_frame],
            condition=IfCondition(rviz),
        ),
    ]

def generate_launch_description():
    rover_description_dir = FindPackageShare('rover_description')

    declared_arguments = [
        DeclareLaunchArgument(
            name='arm', 
            default_value='True',
            description='Include arm URDF in robot_description?',
        ),
        DeclareLaunchArgument(
            name='arm_urdf_path', 
            default_value=PathJoinSubstitution([rover_description_dir, 'waratah_arm', 'urdf', 'arm.urdf.xacro']),
            description='Absolute path to arm urdf file',
        ),
        DeclareLaunchArgument(
            name='rover', 
            default_value='True',
            description='Include rover URDF in robot_description?',
        ),
        DeclareLaunchArgument(
            name='rover_urdf_path', 
            default_value=PathJoinSubstitution([rover_description_dir, 'banksia', 'urdf', 'rover.urdf.xacro']),
            description='Absolute path to rover urdf file',
        ),
        DeclareLaunchArgument(
            name='rviz', 
            default_value='True',
            description='Launch rviz?',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
