from functools import reduce

from launch import LaunchDescription, Substitution, SomeSubstitutionsType
from launch.actions import DeclareLaunchArgument, ExecuteProcess
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, PythonExpression
from launch.utilities import normalize_to_list_of_substitutions
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    param_dir = LaunchConfiguration("param-dir")
    platform = LaunchConfiguration("platform")
    payload = LaunchConfiguration("payload")
    port = LaunchConfiguration("port")

    node_parameters = [
        _substitute_if_not_empty(param_dir, PathJoinSubstitution([param_dir, "directory.yaml"])),
        _substitute_if_not_empty(param_dir, PathJoinSubstitution([param_dir, "streamer.yaml"])),
        _substitute_if_not_empty(param_dir, PathJoinSubstitution([param_dir, "platform", platform, "core.yaml"])),
        _substitute_if_not_empties(
            (param_dir, payload),
            PathJoinSubstitution([param_dir, "platform", platform, "payload", _cat_substitutions([payload, ".yaml"])]),
        ),
    ]

    return LaunchDescription(
        [
            DeclareLaunchArgument(
                "param-dir",
                default_value=PathJoinSubstitution([FindPackageShare("cameras"), "params"]),
                description="The path to the directory holding camera parameter files.",
            ),
            DeclareLaunchArgument(
                "platform",
                default_value="rover",
                description="The target platform.",
            ),
            DeclareLaunchArgument(
                "payload",
                default_value="",
                description="The payload type.",
            ),
            DeclareLaunchArgument(
                "port",
                default_value="8443",
                description="Specify a port for the gst-webrtc-signalling-server",
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
                executable="cameras_directory_service",
                parameters=node_parameters,
            ),
            Node(
                package="cameras",
                executable="cameras_streamer_service",
                parameters=node_parameters,
            ),
        ]
    )


def _cat_substitutions(substitutions: SomeSubstitutionsType) -> Substitution:
    return PythonExpression(["'", *normalize_to_list_of_substitutions(substitutions), "'"])


def _substitute_if_not_empty(
    value: str | Substitution,
    substitution: str | Substitution,
) -> Substitution:
    # https://github.com/ros2/launch/issues/290#issuecomment-520643662
    return PythonExpression(["'", substitution, "'", "if '' != '", value, "' else ''"])


def _substitute_if_not_empties(values: SomeSubstitutionsType, substitution: str | Substitution) -> Substitution:
    return reduce(
        lambda result, value: _substitute_if_not_empty(value, result),
        reversed(normalize_to_list_of_substitutions(values)),
        substitution,
    )
