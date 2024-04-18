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

    #General Configuartions
    use_sim_time = LaunchConfiguration('use_sim_time')
    
    #Simulation Configurations
    gazebo = LaunchConfiguration('gazebo').perform(context).lower() == "true"
    use_true_odometry = LaunchConfiguration('use_true_odometry').perform(context).lower() == "true"

    use_ekf = LaunchConfiguration('use_ekf')
    use_slam = LaunchConfiguration('use_slam')
    use_vo = LaunchConfiguration('use_vo')

    #Params File Configurations
    ekf_params = LaunchConfiguration('ekf_params')
    ukf_params = LaunchConfiguration('ukf_params')

    auto_bringup_path = get_package_share_path('auto_bringup')

    if LaunchConfiguration("use_ukf").perfom(context).lower() == "true":
        filter_type = 'ukf'
        params_file = ukf_params.perform(context)
    elif LaunchConfiguration("use_ukf").perfom(context).lower() == "false":
        filter_type = 'ekf'
        params_file = ekf_params.perform(context)
    else:
        raise ValueError("use_ukf must be either True or False")

    true_odom_params = {
        "odom0": "/odom/gazebo",
        "odom0_relative": False,
        "odom0_config": [True,  True,  True,
                         True,  True,  True,
                         True,  False, False,
                         False, False, False,
                         False, False, False],
        "odom0_relative": True,
    }

    robot_localisation_node = Node(
        package='robot_localization',
        executable= filter_type + '_node',
        name= filter_type + '_filter_node',
        output='screen',
        parameters=[(params_file), 
                    {"use_sim_time": use_sim_time},
                    true_odom_params if use_true_odometry and gazebo else {}],
    )

    slam_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource((auto_bringup_path / 'launch' / 'rtabmap.launch.py').as_posix()),
        condition=IfCondition(use_slam),
        launch_arguments={
            'use_sim_time': use_sim_time,
        }.items()
    )

    static_transform = Node(
        package='tf2_ros',
        executable='static_transform_publisher',
        condition=UnlessCondition(use_sim_time),
        name='static_transform_publisher',
        output='screen',
        arguments=['0', '0', '0', '0', '0', '0', 'map', 'odom'],
    )
    
    return [
        robot_localisation_node,
        slam_cmd,
    ]
    
def generate_launch_description():
    use_sim_time_arg = DeclareLaunchArgument(
        'use_sim_time',
        default_value='False',
        description='Use simulation clock if true')

    use_real_odom_arg = DeclareLaunchArgument(
        'use_true_odometry',
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
        default_value=PathJoinSubstitution(FindPackageShare("auto_bringup"),'params','ekf.yaml')
    )

    ukf_param_arg = DeclareLaunchArgument(
        'ukf_params_file',
        default_value=PathJoinSubstitution(FindPackageShare("auto_bringup"),'params','ukf.yaml')
    )

    declared_arguments = [
        use_ukf_arg,
        use_sim_time_arg,
        use_real_odom_arg,
        ekf_param_arg,
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
