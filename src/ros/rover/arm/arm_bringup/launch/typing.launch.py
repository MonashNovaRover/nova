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
from launch.conditions import IfCondition
from launch.actions import DeclareLaunchArgument, OpaqueFunction, GroupAction
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, IfElseSubstitution

from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

from os.path import expanduser

typing_dir = expanduser('~') + '/nova/src/ros/rover/arm/arm_bringup'

def launch_setup(context, *args, **kwargs):
    old_arm = LaunchConfiguration('old_arm').perform(context)
    auto_mode = LaunchConfiguration('auto_mode')
    params = LaunchConfiguration('typing_params')
    localiser_params = LaunchConfiguration('localiser_params')
    aruco_params = LaunchConfiguration('aruco_params')

    base_frame = "arm_kinematics_origin"
    if old_arm.lower() in ["true", "t", "1"]:
        base_frame = "arm_link"

    return [
        Node(
            package='auto_typing',
            executable='keyboard_localiser.py',
            parameters=[localiser_params, {"base_frame": base_frame}, {"using_auto": auto_mode}]
        ),
        Node(
            package='auto_typing',
            executable='typing_sequencer.py',
            parameters=[params, {"base_frame": base_frame}]
        ),
        # Note: Use can start vcan1 if testing in sim.
        Node(
            package='auto_typing',
            executable='pokey.py',
            parameters=[params]
        ),
        GroupAction(
            condition=IfCondition(auto_mode),
            actions=[
                Node(
                    package='auto_typing',
                    executable='camera_info_publisher.py',
                    parameters=[params],
                ),
                Node(
                    package='aruco_opencv',
                    executable='aruco_tracker_autostart',
                    arguments=['--ros-args', '--params-file', aruco_params],
                ),
            ]
        ),
    ]


def generate_launch_description():
    local = LaunchConfiguration('local')

    arm_bringup_dir = IfElseSubstitution(local,
        PathJoinSubstitution([typing_dir]),
        FindPackageShare('arm_bringup')
    )

    declared_arguments = [
        DeclareLaunchArgument(
            name='local',
            default_value='True',
            description='Use local source directories instead of the nix store.',
        ),
        DeclareLaunchArgument(
            name='typing_params',
            default_value=PathJoinSubstitution([arm_bringup_dir, 'params', 'typing.yaml']),
            description='Path to typing params file',
        ),
        DeclareLaunchArgument(
            name='localiser_params',
            default_value=PathJoinSubstitution([arm_bringup_dir, 'params', 'localisers.yaml']),
            description='Path to localiser params file',
        ),
        DeclareLaunchArgument(
            name='aruco_params',
            default_value=PathJoinSubstitution([arm_bringup_dir, 'params', 'aruco', 'typing_tracker.yaml']),
            description='Path to ArUco tracker params file',
        ),
        DeclareLaunchArgument(
            name='old_arm',
            default_value='False',
            description='Switch to old arm mode if true',
        ),
        DeclareLaunchArgument(
            name='auto_mode',
            default_value='True',
            description='Publish fixed keyboard transform (as specified in yaml) if false, known as manual mode',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )