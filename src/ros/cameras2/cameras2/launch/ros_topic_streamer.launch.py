from functools import reduce

from launch import LaunchDescription, Substitution, SomeSubstitutionsType
from launch.actions import DeclareLaunchArgument, ExecuteProcess
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, PythonExpression
from launch.utilities import normalize_to_list_of_substitutions
from launch.conditions import IfCondition
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    params = LaunchConfiguration("params")
    signalling = LaunchConfiguration("signalling")

    return LaunchDescription(
        [
            DeclareLaunchArgument(
                "params",
                default_value=PathJoinSubstitution([FindPackageShare("cameras2"), "params", "ros_streamer.yaml"]),
                description="The path to the ros_streaming param file",
            ),
            DeclareLaunchArgument(
                "signalling",
                default_value='False',
                description="If to launch the gst-webrtc-signalling-server",
            ),
            ExecuteProcess(
                condition=IfCondition(signalling),
                cmd=["gst-webrtc-signalling-server"],
                additional_env={
                    "WEBRTCSINK_SIGNALLING_SERVER_LOG": "warn",
                },
                output="screen",
            ),
            Node(
                package="cameras2",
                executable="camera_ros_streamer",
                parameters=[params],
            ),
        ]
    )