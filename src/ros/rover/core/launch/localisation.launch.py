"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the rover to publish the urdf
    static transforms and associated joint states
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - robot_state_publisher
  - rover_state_publisher
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	core
CREATION:	27/04/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

from ament_index_python.packages import get_package_share_path

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch.conditions import IfCondition, UnlessCondition
from launch.actions import IncludeLaunchDescription, DeclareLaunchArgument
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

from launch_ros.actions import Node

# Generate the launch file with all inputs
def generate_launch_description():
    # Declare a launch configuration argument of the name "t265"
    from_tracking_cam = LaunchConfiguration('t265')

    # Define a launch argument that runs another launch file
    urdf_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            FindPackageShare('core'), "/launch", "/urdf.launch.py"
        ])
    )

    # Define t265 argument default and description
    tracking_cam_arg = DeclareLaunchArgument(
        name='t265',
        default_value='False',
        description="Set to 'True' to run localisation from the t265 tracking camera"
    )

    # Define a launch argument to launch a ros node
    world_to_map_node = Node(
            package='tf2_ros',
            executable='static_transform_publisher',
            arguments=['0', '0', '0', '0', '0', '0', 'world', 'map'],
            output='screen',
            emulate_tty=True
        )

    # Launch URC pose converter unless we're using tracking cam
    pose_converter_node = Node(
        package='autonomous',
        executable='pose_converter.py',
        output='screen',
        emulate_tty=True,
        condition=UnlessCondition(from_tracking_cam)
    )

    imu_node = Node(
        package='imu',
        executable='imu_node',
        output='screen',
        emulate_tty=True,
        condition=UnlessCondition(from_tracking_cam)
    )

    gps_pub_node = Node(
        package='electronics',
        executable='gps_publisher.py',
        output='screen',
        emulate_tty=True,
        condition=UnlessCondition(from_tracking_cam)
    )

    gps_sub_node = Node(
        package='electronics',
        executable='base_gps_sub.py',
        output='screen',
        emulate_tty=True,
        condition=UnlessCondition(from_tracking_cam)
    )

    pose_converter_t265_node = Node(
        package='autonomous',
        executable='pose_converter_ARC.py',
        output='screen',
        emulate_tty=True,
        condition=IfCondition(from_tracking_cam)
    )

    t265_node = Node(
        package='autonomous',
        executable='tracking_camera.py',
        output='screen',
        emulate_tty=True,
        condition=IfCondition(from_tracking_cam)
    )
    return LaunchDescription([
        urdf_launch,
        tracking_cam_arg,
        imu_node,
        gps_sub_node,
        gps_pub_node,
        t265_node,
        world_to_map_node,
        pose_converter_node,
        pose_converter_t265_node,
    ])
