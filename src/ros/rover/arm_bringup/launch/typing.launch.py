'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the rover to run nodes 
    associated with auto typing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - robot_state_publisher
  - rover_state_publisher
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	arm_bringup
CREATION:	25/05/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.substitutions import Command, LaunchConfiguration, PathJoinSubstitution

from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    old_arm = LaunchConfiguration('old_arm').perform(context)
    params = LaunchConfiguration('typing_params')

    base_frame = "arm_kinematics_origin"
    if old_arm.lower() in ["true", "t", "1"]:
        base_frame = "arm_link"

    return [
        Node(
            package='auto_typing',
            executable='keyboard_localiser.py',
            parameters=[params, {"base_frame": base_frame}]
        ),
        Node(
            package='auto_typing',
            executable='typing_sequencer.py',
            parameters=[params, {"base_frame": base_frame}]
        )
    ]


def generate_launch_description():
    arm_bringup_dir = FindPackageShare('arm_bringup')

    declared_arguments = [
        DeclareLaunchArgument(
            name='typing_params',
            default_value=PathJoinSubstitution([arm_bringup_dir, 'params', 'typing.yaml']),
            description='Absolute path to robot urdf file',
        ),
        DeclareLaunchArgument(
            name='old_arm',
            default_value='True',
            description='Switch to old arm mode if true',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )