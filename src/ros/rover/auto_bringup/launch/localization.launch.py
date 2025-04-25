'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This launch file is responsible for localisation.
When gps is False, a single robot_localization node 
    is launched which fuses all sources of information 
    for odom to base transform
When GPS is True, not only does is the odom to base transform
    but the odom to map transform is published too.
    The GPS is fused with IMU and wheel odom in a 
    second robot_localization node.
To ensure accuracy of GPS it is transformed using 
    navsat_transform_node which fuses with IMU
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - robot_localization
    OR
  - robot_localization (local)
  - robot_localization (global)
  - navsat_transform_node
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	auto_bringup
CREATION:	UNKNOWN
EDITED:     24/04/2025
EDITED BY: Taaj Street, Kabilan Velmurugan 
           Sujatha, Anthony Lew, Victor Bartlinski
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction, GroupAction
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch.conditions import IfCondition, UnlessCondition
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    gazebo = (LaunchConfiguration('gazebo').perform(context).lower() == 'true')
    gps = LaunchConfiguration('gps').perform(context)
    rl_params = LaunchConfiguration('rl_params').perform(context)
    use_ukf = (LaunchConfiguration('use_ukf').perform(context).lower() == 'true')

    if use_ukf:
        filter_type = 'ukf'
    elif not use_ukf:
        filter_type = 'ekf'
    else:
        raise ValueError('use_ukf must be either True or False')

    return [
        Node(
            condition=UnlessCondition(gps),
            package='robot_localization',
            executable=f'{filter_type}_node',
            name=f'{filter_type}_filter_node',
            output='screen',
            parameters=[rl_params, {'use_sim_time': gazebo}],
        ),
        GroupAction(
            # Why is there more nodes for GPS?
            # https://docs.ros.org/en/api/robot_localization/html/integrating_gps.html
            condition=IfCondition(gps),
            actions=[
                Node(
                    package='robot_localization',
                    executable='ekf_node',
                    name='ekf_filter_node_odom',
                    output='screen',
                    parameters=[rl_params, {'use_sim_time': gazebo}],
                    remappings=[('odometry/filtered', 'odometry/local')],
                ),
                Node(
                    package='robot_localization',
                    executable='ekf_node',
                    name='ekf_filter_node_map',
                    output='screen',
                    parameters=[rl_params, {'use_sim_time': gazebo}],
                    remappings=[('odometry/filtered', 'odometry/global')],
                ),
                Node(
                    package='robot_localization',
                    executable='navsat_transform_node',
                    name='navsat_transform',
                    output='screen',
                    parameters=[rl_params, {'use_sim_time': gazebo}],
                    remappings=[
                        ('odometry/filtered', 'odometry/global'),
                        ('gps/fix', 'fix'),
                        ('imu', 'oak/imu/transformed')],
                ),
            ],
        ),
    ]

def generate_launch_description():
    auto_bringup_dir = FindPackageShare('auto_bringup')

    declared_arguments = [
        DeclareLaunchArgument(
            name='gazebo',
            default_value='False',
            description='Flag if using gazebo',
        ),
        DeclareLaunchArgument(
            name='gps',
            default_value='True',
            description='Fuse GPS?',
        ),
        DeclareLaunchArgument(
            name='rl_params',
            default_value=PathJoinSubstitution([auto_bringup_dir,'params','rl_urc.yaml']),
            description='',
        ),
        DeclareLaunchArgument(
            name='use_ukf',
            default_value='False',
            description='Use UKF (True) or EKF (False)',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
