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
    load_map = LaunchConfiguration('load_map')

    gazebo_odom_params = {
        "use_sim_time": use_sim_time, 
        "odom0": "/odom/gazebo",
        "odom0_relative": False,
        "odom0_config": [True,  True,  True,  \
                       True, True, True,      \
                       True, False, False,      \
                       False, False, False,      \
                       False, False, False],
        "odom1_relative": True,
    }

    use_sim_time_arg = DeclareLaunchArgument(
        'use_sim_time',
        default_value='False',
        description='Use simulation (Gazebo) clock if true')

    use_real_odom_arg = DeclareLaunchArgument(
        'use_real_odometry',
        default_value='true',
        description='True to use robot_localisation odometry, False to use p3d gazebo plugin'
    )

    load_map_arg = DeclareLaunchArgument(
        'load_map',
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

    ukf_localisation_odom = Node(
        condition=IfCondition(use_real_odometry),
        package='robot_localization',
        executable='ukf_node',
        name='ukf_filter_node',
        output='screen',
        parameters=[(get_package_share_path("core") / 'params' / 'ukf.yaml').as_posix(), {"use_sim_time": use_sim_time}],
    )
    slam_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource((get_package_share_path("core") / 'launch' / 'rtabmap.launch.py').as_posix()),
        launch_arguments={
            'use_sim_time': use_sim_time,
            'load_map': load_map,
        }.items()
    )
    
    return LaunchDescription([
        use_sim_time_arg,
        use_real_odom_arg,
        load_map_arg,
        ukf_localisation_gazebo,
        ukf_localisation_odom,
        slam_cmd,
    ])
