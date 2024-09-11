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
import os

from ament_index_python.packages import get_package_share_path

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.substitutions import LaunchConfiguration, PythonExpression, OrSubstitution, AndSubstitution, NotSubstitution, PathJoinSubstitution
from launch.conditions import IfCondition, UnlessCondition
from launch.actions import IncludeLaunchDescription, DeclareLaunchArgument
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

# Generate the launch file with all inputs
# TODO: Add IMU, GPS, and any other localisation nodes


def launch_setup(context, *args, **kwargs):

    # General Configuartions
    use_sim_time = LaunchConfiguration('use_sim_time')

    # Simulation Configurations
    use_real_odometry = LaunchConfiguration('use_real_odometry').perform(context).lower() == "true"

    use_slam = LaunchConfiguration('use_slam')
    use_vo = LaunchConfiguration('use_vo')
    gps = LaunchConfiguration('gps')

    # Params File Configurations
    ekf_params = LaunchConfiguration('ekf_params_file')
    ukf_params = LaunchConfiguration('ukf_params_file')

    auto_bringup_path = get_package_share_path('auto_bringup')

    if LaunchConfiguration("use_ukf").perform(context).lower() == "true":
        filter_type = 'ukf'
        params_file = ukf_params.perform(context)
    elif LaunchConfiguration("use_ukf").perform(context).lower() == "false":
        filter_type = 'ekf'
        params_file = ekf_params.perform(context)
    else:
        raise ValueError("use_ukf must be either True or False")

    real_odom_params = {
        "odom0": "/odom/gazebo",
        "odom0_relative": False,
        "odom0_config": [True, True, True,
                         True, True, True,
                         True, False, False,
                         False, False, False,
                         False, False, False],
        "odom0_relative": True,
    }

    robot_localisation_node = Node(
        condition=UnlessCondition(gps),
        package='robot_localization',
        executable=filter_type + '_node',
        name=filter_type + '_filter_node',
        output='screen',
        parameters=[(params_file), 
                    {"use_sim_time": use_sim_time},
                    real_odom_params if use_real_odometry else {}],
    )

    gps_localisation_odom = Node(
        condition=IfCondition(gps),
        package='robot_localization',
        executable='ekf_node',
        name='ekf_filter_node_odom',
        output='screen',
        parameters=[(LaunchConfiguration('rl_params_file').perform(context)), 
                    {"use_sim_time": use_sim_time}],
        remappings=[("odometry/filtered", "odometry/local")]
    )

    gps_localisation_map = Node(
        condition=IfCondition(gps),
        package='robot_localization',
        executable='ekf_node',
        name='ekf_filter_node_map',
        output='screen',
        parameters=[(LaunchConfiguration('rl_params_file').perform(context)), {"use_sim_time": use_sim_time}],
        remappings=[("odometry/filtered", "odometry/global")]
    )

    navsat_transform_node = Node(
        condition=IfCondition(gps),
        package='robot_localization',
        executable='navsat_transform_node',
        name='navsat_transform',
        output='screen',
        parameters=[(LaunchConfiguration('rl_params_file').perform(context)), {"use_sim_time": use_sim_time}],
        remappings=[
								("odometry/filtered", "odometry/global"),
								("gps/fix", "fix"),
								("imu", "oak/imu/transformed"),
				]
    )

    slam_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource((auto_bringup_path / 'launch' / 'rtabmap.launch.py').as_posix()),
        condition=IfCondition(use_slam),
        launch_arguments={
            'use_sim_time': use_sim_time,
        }.items()
    )

    static_transform_node = Node(
        package='tf2_ros',
        executable='static_transform_publisher',
        condition=UnlessCondition(OrSubstitution(use_slam, gps)),
        name='static_transform_publisher',
        output='screen',
        arguments=['0', '0', '0', '0', '0', '0', 'map', 'odom'],
    )

    return [
        robot_localisation_node,
        gps_localisation_odom,
        gps_localisation_map,
        navsat_transform_node,
        static_transform_node,
        slam_cmd,
    ]


def generate_launch_description():
    use_sim_time_arg = DeclareLaunchArgument(
        'use_sim_time',
        default_value='False',
        description='Use simulation clock if true')

    use_real_odom_arg = DeclareLaunchArgument(
        'use_real_odometry',
        default_value='False',
        description='Use the ground truth odometry from gazebo'
    )

    use_ukf_arg = DeclareLaunchArgument(
        'use_ukf',
        default_value='False',
        description='Use UKF (True) or EKF (False)'
    )

    ekf_param_arg = DeclareLaunchArgument(
        'ekf_params_file',
        default_value=PathJoinSubstitution([FindPackageShare("auto_bringup"), 'params', 'ekf.yaml']),
        description='Params file for ekf filter node'
    )

    ukf_param_arg = DeclareLaunchArgument(
        'ukf_params_file',
        default_value=PathJoinSubstitution([FindPackageShare("auto_bringup"), 'params', 'ukf.yaml']),
        description='Params file for ukf filter node'
    )

    use_slam = DeclareLaunchArgument(
        'use_slam',
        default_value='False',
        description='use slam for map->odom transform'

    )

    rl_param_arg = DeclareLaunchArgument(
        'rl_params_file',
        default_value=PathJoinSubstitution([FindPackageShare("auto_bringup"),'params','rl.yaml'])
    )
    
    gps_arg = DeclareLaunchArgument(
        'gps',
        default_value='True',
        description='Fuse GPS?'
    )

    declared_arguments = [
        use_ukf_arg,
        use_sim_time_arg,
        use_real_odom_arg,
        use_ukf_arg,
        ekf_param_arg,
        ukf_param_arg,
        use_slam,
        rl_param_arg,
        gps_arg
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
