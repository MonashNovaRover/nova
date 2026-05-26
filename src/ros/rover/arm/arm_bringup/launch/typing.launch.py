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
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, AndSubstitution, NotSubstitution

from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    old_arm = LaunchConfiguration('old_arm').perform(context)
    auto_mode = LaunchConfiguration('auto_mode')
    use_realsense = LaunchConfiguration('use_realsense')
    params = LaunchConfiguration('typing_params')
    aruco_params = LaunchConfiguration('aruco_params')

    base_frame = "arm_kinematics_origin"
    if old_arm.lower() in ["true", "t", "1"]:
        base_frame = "arm_link"

    cam_name = LaunchConfiguration('cam_name').perform(context)
    rs = use_realsense.perform(context).lower() in ["true", "t", "1"]

    # When using RealSense: override aruco tracker to use realsense color stream
    # aruco_opencv handles distortion internally via camera_info when image_is_rectified=false
    aruco_overrides = [
        {
            "cam_base_topic": f"/{cam_name}/color/image_raw",
            "image_is_rectified": False,
        }
    ] if rs else []

    return [n for n in [
        Node(
            package='auto_typing',
            executable='keyboard_localiser.py',
            parameters=[params, {"base_frame": base_frame}, {"using_auto": auto_mode}]
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
        # Lifecam: needs camera_info_publisher (no driver-provided intrinsics)
        GroupAction(
            condition=IfCondition(AndSubstitution(auto_mode, NotSubstitution(use_realsense))),
            actions=[
                Node(
                    package='auto_typing',
                    executable='camera_info_publisher.py',
                    parameters=[params],
                ),
            ]
        ),
        GroupAction(
            condition=IfCondition(use_realsense),
            actions=[
                Node(
                    package='tf2_ros',
                    executable='static_transform_publisher',
                    arguments=[
                        '0','0','0',
                        '0','0','0',
                        'image_frame',
                        'd415_link'
                    ]
                )
            ]
        ),
        GroupAction(
            condition=IfCondition(auto_mode),
            actions=[
                Node(
                    package='aruco_opencv',
                    executable='aruco_tracker_autostart',
                    parameters=[aruco_params] + aruco_overrides
                ),
            ]
        ),
    ]
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
            name='aruco_params',
            default_value=PathJoinSubstitution([arm_bringup_dir, 'params', 'typing_aruco_tracker.yaml']),
            description='Absolute path to ArUco tracker params file',
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
        DeclareLaunchArgument(
            name='use_realsense',
            default_value='False',
            description='Use RealSense D415 (skip camera_info_publisher, point aruco_tracker at RealSense infra stream)',
        ),
        DeclareLaunchArgument(
            name='cam_name',
            default_value='d415',
            description='RealSense camera name (used for topic prefix when use_realsense is True).',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )