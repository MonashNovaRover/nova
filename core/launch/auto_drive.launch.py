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
from launch.actions import IncludeLaunchDescription, DeclareLaunchArgument
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
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

    # tf2 static transforms
    tf_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            FindPackageShare("core"), '/launch', '/tf.launch.py'
        ])
    )

    # autonomous nodes
    main_launch = Node(
        package="autonomous",
        node_executable="main.py",
        output="screen",
        parameters=[core_params_path / "auto_params.yaml"],
        emulate_tty=True
    )
    tracking_cam_launch = Node(
        package='autonomous',
        node_executable='tracking_camera.py',
        output='screen',
        parameters=[core_params_path / "auto_params.yaml"],
        emulate_tty=True,
        condition=UnlessCondition(from_rosbag)
    )
    depth_cam_launch = Node(
        package="autonomous",
        node_executable="depth_camera.py",
        output="screen",
        parameters=[core_params_path / "auto_params.yaml"],
        emulate_tty=True,
        condition=UnlessCondition(from_rosbag)
    )
    pose_converter_launch = Node(
        package="autonomous",
        node_executable="pose_converter_ARC.py",
        output="screen",
        parameters=[core_params_path / "auto_params.yaml"],
        emulate_tty=True
    )
    gruc_launch = Node(
        package="autonomous",
        node_executable="GRUC.py",
        output="screen",
        parameters=[core_params_path / "auto_params.yaml"],
        emulate_tty=True
    )
    grup_launch = Node(
        package="autonomous",
        node_executable="GRUP.py",
        output="screen",
        parameters=[core_params_path / "auto_params.yaml"],
        emulate_tty=True
    )
    goal_manager_launch = Node(
        package="autonomous",
        node_executable="goal_manager.py",
        output="screen",
        parameters=[core_params_path / "auto_params.yaml"],
        emulate_tty=True
    )

    return LaunchDescription([
        from_rosbag_arg,
        tf_launch,
        main_launch,
        tracking_cam_launch,
        depth_cam_launch,
        pose_converter_launch,
        gruc_launch,
        grup_launch,
        goal_manager_launch
    ])
