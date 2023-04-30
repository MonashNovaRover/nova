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
from launch.substitutions import LaunchConfiguration
from launch.conditions import IfCondition, UnlessCondition
from launch.actions import DeclareLaunchArgument
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_path


# Generate the launch file with all inputs
def generate_launch_description():
    # Get the path to the parameters files in core package
    core_params_path = get_package_share_path('core') / "params"

    from_rosbag = LaunchConfiguration('from_rosbag')

    from_rosbag_arg = DeclareLaunchArgument(
        name='from_rosbag',
        default_value='False',
        description="Set to 'True' to run autonomous algorithms on camera data we will play from a rosbag"
    )

    # autonomous nodes
    main_launch = Node(
        package="autonomous",
        executable="main.py",
        output="screen",
        parameters=[core_params_path / "auto_params.yaml"],
        emulate_tty=True
    )
    depth_cam_launch = Node(
        package="autonomous",
        executable="depth_camera.py",
        output="screen",
        parameters=[core_params_path / "auto_params.yaml"],
        emulate_tty=True,
        condition=UnlessCondition(from_rosbag)
    )
    stamp_converter_launch = Node(
        package="autonomous",
        executable="stamp_converter.py",
        output="screen",
        parameters=[core_params_path / "auto_params.yaml"],
        emulate_tty=True,
        condition=IfCondition(from_rosbag)
    )
    controller_launch = Node(
        package="autonomous",
        executable="controller.py",
        output="screen",
        parameters=[core_params_path / "auto_params.yaml"],
        emulate_tty=True
    )
    goal_selector_launch = Node(
        package="autonomous",
        executable="goal_selector.py",
        output="screen",
        parameters=[core_params_path / "auto_params.yaml"],
        emulate_tty=True
    )

    return LaunchDescription([
        from_rosbag_arg,
        #main_launch,
        depth_cam_launch,
        stamp_converter_launch,
        controller_launch,
        goal_selector_launch,
    ])
