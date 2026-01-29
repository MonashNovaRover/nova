from functools import reduce

from launch import LaunchDescription, Substitution, SomeSubstitutionsType
from launch.actions import DeclareLaunchArgument, ExecuteProcess
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, PythonExpression
from launch.utilities import normalize_to_list_of_substitutions
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    return LaunchDescription(
        [
            Node(
                package="oakenc",
                executable="oak_camera_directory_service",
                #parameters=node_parameters,
                remappings = [
                    ("/camera_directory/cameras", "/oak_camera_directory/cameras"),
                    ("/camera_directory/discover", "/oak_camera_directory/discover"),
                ]
            ),
        ]
    )


