from functools import reduce

from launch import LaunchDescription, Substitution, SomeSubstitutionsType
from launch.actions import DeclareLaunchArgument, ExecuteProcess
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, PythonExpression, IfElseSubstitution
from launch.utilities import normalize_to_list_of_substitutions
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

from os.path import expanduser


def generate_launch_description():
    local = LaunchConfiguration('local')
    dev = LaunchConfiguration('dev')

    cameras_dir = IfElseSubstitution(local,
        PathJoinSubstitution([expanduser("~") + '/nova/src/ros/cameras/cameras3']),
        FindPackageShare('cameras')
    ) 

    params = LaunchConfiguration("param_dir")
    directory_params = PathJoinSubstitution([params, "directory.yaml"])
    streamer_params = PathJoinSubstitution([params, "streamer.yaml"])
    platform = LaunchConfiguration("platform")
    task = LaunchConfiguration("task")
    preset = LaunchConfiguration("preset")
    port = LaunchConfiguration("port")
    autostart = LaunchConfiguration("autostart")

    return LaunchDescription(
        [
            DeclareLaunchArgument(
                name='local',
                default_value='True',
                description='Whether to use local directories instead of the nix store.',
            ),
            DeclareLaunchArgument(
                name='dev',
                default_value='False',
                description='Whether to use development parameter directory.',
            ),

            DeclareLaunchArgument(
                "param_dir",
                default_value=IfElseSubstitution(dev,
                    PathJoinSubstitution([cameras_dir, "params/dev"]),
                    PathJoinSubstitution([cameras_dir, "params"])
                ),
                description="The path to the directory holding camera parameter files.",
            ),
            DeclareLaunchArgument(
                "platform",
                default_value="orin",
                description="The platform type. Used for specifying serial overrides for Camera Directory Node",
            ),
            DeclareLaunchArgument(
                "task",
                default_value="delivery",
                description="The task type. Used for specifying serial overrides for Camera Directory Node",
            ),
                DeclareLaunchArgument(
                "preset",
                default_value="default",
                description="The global profile type. Used for specifying the profile to use for each individual serial",
            ),
            DeclareLaunchArgument(
                "port",
                default_value="8443",
                description="Specify a port for the gst-webrtc-signalling-server",
            ),
            DeclareLaunchArgument(
                "autostart",
                default_value="true",
                description="Specify whether to start camera streaming automatically",
            ),

            ExecuteProcess(
                cmd=["gst-webrtc-signalling-server", "--port", port],
                additional_env={
                    "WEBRTCSINK_SIGNALLING_SERVER_LOG": "warn",
                },
                output="screen",
            ),
            Node(
                package="cameras",
                executable="camera_directory_service",
                parameters=[{'platform': platform, 'task': task}, directory_params],
            ),
            Node(
                package="cameras",
                executable="camera_streamer_service",
                parameters=[{'task': task, 'preset': preset, 'autostart': autostart}, streamer_params],
            ),
        ]
    )
