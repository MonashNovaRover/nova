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
from launch.substitutions import LaunchConfiguration, PythonExpression
from launch.conditions import IfCondition, UnlessCondition
from launch.actions import IncludeLaunchDescription, DeclareLaunchArgument
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node

from launch_ros.actions import Node

# Generate the launch file with all inputs
# TODO: Add IMU, GPS, and any other localisation nodes
def generate_launch_description():
    # Declare a launch configuration argument of the name "t265"
    use_sim_time = LaunchConfiguration('use_sim_time')
    slam = LaunchConfiguration('slam')

    use_sim_time_arg = DeclareLaunchArgument(
        'use_sim_time',
        default_value='true',
        description='Use simulation (Gazebo) clock if true')

    use_slam_arg = DeclareLaunchArgument(
        'slam',
        default_value='true',
        description='True to use RTABMAP, False to use robot_localization'
    )

    ukf_localisation = Node(
        package='robot_localization',
        executable='ukf_node',
        name='ukf_filter_node',
        output='screen',
        parameters=[(get_package_share_path("core") / 'params' / 'ukf.yaml').as_posix(), {"use_sim_time": use_sim_time}],
    )

    # TODO: Implement real SLAM rather than publishing a static transform. This is currently necessary for things to be visible in the RVIZ 'map' frame
    slam_cmd = Node(
        condition=UnlessCondition(slam),
        package='tf2_ros',
        executable='static_transform_publisher',
        arguments = ['--x', '0', '--y', '0', '--z', '0', '--yaw', '0', '--pitch', '0', '--roll', '0', '--frame-id', 'map', '--child-frame-id', 'odom']
    )

    slam_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource((get_package_share_path("core") / 'launch' / 'rtabmap_launch.py').as_posix()),
        condition=IfCondition(slam),
        launch_arguments={
            'use_sim_time': use_sim_time,
        }.items()
    )
    
    return LaunchDescription([
        use_sim_time_arg,
        use_slam_arg,
        ukf_localisation,
        slam_cmd,
    ])
