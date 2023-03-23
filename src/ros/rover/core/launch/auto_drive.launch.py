"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the rover to start all
   rover control scripts.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - control/drive/drive_inputs      [drive_cmd]
  - control/drive/driver            [driver]
  - tf2 static transforms
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	core
CREATION:	15/12/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

# Include the required launch parameters
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from ament_index_python.packages import get_package_share_path


# Generate the launch file with all inputs
def generate_launch_description():
    # Get the path to the parameters files in core package
    core_params_path = get_package_share_path('core') / "params"

    return LaunchDescription([
        # tf2 static transforms
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource([
                FindPackageShare("core"), '/launch', '/tf.launch.py'
            ])
        ),
        # autonomous nodes
        Node(
            package="autonomous",
            node_executable="main.py",
            output="screen",
            parameters=[core_params_path / "auto_params.yaml"],
            emulate_tty=True
        ),
        Node(
            package='autonomous',
            node_executable='tracking_camera.py',
            output='screen',
            parameters=[core_params_path / "auto_params.yaml"],
            emulate_tty=True
        ),
        Node(
            package="autonomous",
            node_executable="wheel_odometry.py",
            output="screen",
            parameters=[core_params_path / "auto_params.yaml"],
            emulate_tty=True
        ),
        Node(
            package="autonomous",
            node_executable="depth_camera.py",
            output="screen",
            parameters=[core_params_path / "auto_params.yaml"],
            emulate_tty=True
        ),
        Node(
            package="autonomous",
            node_executable="pose_converter_ARC.py",
            output="screen",
            parameters=[core_params_path / "auto_params.yaml"],
            emulate_tty=True
        ),
        Node(
            package="autonomous",
            node_executable="GRUC.py",
            output="screen",
            parameters=[core_params_path / "auto_params.yaml"],
            emulate_tty=True
        ),
        Node(
            package="autonomous",
            node_executable="GRUP.py",
            output="screen",
            parameters=[core_params_path / "auto_params.yaml"],
            emulate_tty=True
        ),
        Node(
            package="autonomous",
            node_executable="goal_manager.py",
            output="screen",
            parameters=[core_params_path / "auto_params.yaml"],
            emulate_tty=True
        ),
    ])
