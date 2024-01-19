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
from launch.substitutions import LaunchConfiguration, PythonExpression, OrSubstitution, AndSubstitution
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
    use_real_odometry = LaunchConfiguration('use_real_odometry')
    localization = LaunchConfiguration('localization')

    gazebo_odom_params = {
        "use_sim_time": use_sim_time, 
        "odom0": "/odom/gazebo",
        "odom0_relative": False,
        "odom0_config": [True,  True,  True,  \
                       True, True, True,      \
                       True, False, False,      \
                       False, False, False,      \
                       False, False, False],
    }

    use_sim_time_arg = DeclareLaunchArgument(
        'use_sim_time',
        default_value='true',
        description='Use simulation (Gazebo) clock if true')

    use_real_odom_arg = DeclareLaunchArgument(
        'use_real_odometry',
        default_value='true',
        description='True to use robot_localisation odometry, False to use p3d gazebo plugin'
    )

    localization_arg = DeclareLaunchArgument(
        'localization',
        default_value='false',
        description='Localize the rover in a pre-existing map'
    )

    ukf_localisation_gazebo = Node(
        condition=UnlessCondition(use_real_odometry),
        package='robot_localization',
        executable='ukf_node',
        name='ukf_filter_node',
        output='screen',
        parameters=[(get_package_share_path("core") / 'params' / 'ukf.yaml').as_posix(), gazebo_odom_params],
    )

    # TODO: Get normal odom working
    # ukf_localisation_odom = Node(
    #     condition=IfCondition(AndSubstitution(use_real_odometry)),
    #     package='robot_localization',
    #     executable='ukf_node',
    #     name='ukf_filter_node',
    #     output='screen',
    #     parameters=[(get_package_share_path("core") / 'params' / 'ukf.yaml').as_posix(), {"use_sim_time": use_sim_time, "odom0": "/odom"}],
    # )
    ukf_localisation_odom = Node(
        condition=IfCondition(use_real_odometry),
        package='tf2_ros',
        executable='static_transform_publisher',
        arguments = ['--x', '0', '--y', '0', '--z', '0', '--yaw', '0', '--pitch', '0', '--roll', '0', '--frame-id', 'odom', '--child-frame-id', 'base_link']
    )

    slam_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource((get_package_share_path("core") / 'launch' / 'rtabmap_launch.py').as_posix()),
        launch_arguments={
            'use_sim_time': use_sim_time,
            'localization': localization,
        }.items()
    )
    
    return LaunchDescription([
        use_sim_time_arg,
        use_real_odom_arg,
        localization_arg,
        ukf_localisation_gazebo,
        ukf_localisation_odom,
        slam_cmd,
    ])
