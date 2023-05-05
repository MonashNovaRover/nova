from launch import LaunchDescription, Substitution, SomeSubstitutionsType
from launch.actions import DeclareLaunchArgument, ExecuteProcess
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, PythonExpression
from launch.utilities import normalize_to_list_of_substitutions
from launch_ros.actions import Node


def generate_launch_description():
    param_dir = LaunchConfiguration("param-dir")
    platform = LaunchConfiguration("platform")

    node_parameters = [
        _substitute_if_not_empty(param_dir, PathJoinSubstitution([param_dir, "core.yaml"])),
        _substitute_if_not_empty(
            param_dir, PathJoinSubstitution([param_dir, "platform", _cat_substitutions([platform, ".yaml"])])
        ),
    ]

    return LaunchDescription(
        [
            DeclareLaunchArgument(
                "param-dir",
                default_value="",
                description="The path to the directory holding camera parameter files.",
            ),
            DeclareLaunchArgument(
                "platform",
                default_value="local",
                description="The target platform.",
            ),
            ExecuteProcess(
                cmd=["gst-webrtc-signalling-server"],
                additional_env={
                    "WEBRTCSINK_SIGNALLING_SERVER_LOG": "warn",
                },
                output="screen",
            ),
            Node(
                package="cameras2",
                executable="camera_directory_service",
                parameters=node_parameters,
            ),
            Node(
                package="cameras2",
                executable="camera_streamer_service",
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
