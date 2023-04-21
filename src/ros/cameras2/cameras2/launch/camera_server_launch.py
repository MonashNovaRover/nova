from launch import LaunchDescription
from launch.actions import ExecuteProcess
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription(
        [
            ExecuteProcess(cmd=["gst-webrtc-signalling-server"]),
            Node(
                package="cameras2",
                executable="camera_directory_service",
                namespace="camera_streamer_deps",
            ),
            Node(
                package="cameras2",
                executable="camera_streamer_service",
                remappings=[("camera_directory_service", "camera_streamer_deps/camera_directory_service")],
            ),
        ]
    )
