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
  - robot_localization/ekf_node
    OR
  - robot_localization/ekf_node (local)
  - robot_localization/ekf_node (global)
  - robot_localization/navsat_transform_node
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	auto_bringup
CREATION:	UNKNOWN
EDITED:     05/01/2026
EDITED BY: Taaj Street, Kabilan Velmurugan 
           Sujatha, Anthony Lew, Victor Bartlinski,
           Terry Tian
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction, GroupAction
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch.conditions import IfCondition, UnlessCondition
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    # package directories
    auto_bringup_dir = FindPackageShare('auto_bringup')

    comp = LaunchConfiguration('comp').perform(context).lower()

    # comp agnostic arguments
    gazebo = LaunchConfiguration('gazebo')

    # comp defaults
    if comp == 'arch':
        rl_params = PathJoinSubstitution([auto_bringup_dir, 'params', 'rl_arch.yaml'])
        gps = 'False'
    elif comp == 'urc':
        rl_params = PathJoinSubstitution([auto_bringup_dir, 'params', 'rl_urc.yaml'])
        gps = 'True'
    else:
        raise ValueError('Invalid comp value')
    
    # comp defaults overrides
    if LaunchConfiguration('rl_params').perform(context) != '':
        rl_params = LaunchConfiguration('rl_params')
    if LaunchConfiguration('gps').perform(context) != '':
        gps = LaunchConfiguration('gps')

    return [
        Node(
            condition=IfCondition(str(comp == 'arch')),
            package='robot_localization',
            executable='ekf_node',
            name='ekf_filter_node',
            output='screen',
            parameters=[rl_params, {'use_sim_time': gazebo}],
        ),
        GroupAction(
            condition=IfCondition(str(comp == 'urc')),
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
                    condition=UnlessCondition(gps),
                    package='tf2_ros',
                    executable='static_transform_publisher',
                    name='static_transform_publisher',
                    output='screen',
                    arguments=['0', '0', '0', '0', '0', '0', 'map', 'odom'],
                ),
                # Why is there more nodes for GPS?
                # https://docs.ros.org/en/api/robot_localization/html/integrating_gps.html
                Node(
                    condition=IfCondition(gps),
                    package='robot_localization',
                    executable='navsat_transform_node',
                    name='navsat_transform',
                    output='screen',
                    parameters=[rl_params, {'use_sim_time': gazebo}],
                    remappings=[('odometry/filtered', 'odometry/global'),
                                ('gps/fix', 'gps_rover/fix'),
                                ('imu', 'oak/imu/transformed')],
                ),
            ],
        ),
    ]

def generate_launch_description():
    auto_bringup_dir = FindPackageShare('auto_bringup')

    declared_arguments = [
        DeclareLaunchArgument(
            name='comp',
            default_value='arch',
            description='ARCh or URC',
        ),
        # comp agnostic arguments
        DeclareLaunchArgument(
            name='gazebo',
            default_value='False',
            description='Flag if using gazebo',
        ),
        # arguments with comp defaults
        DeclareLaunchArgument(
            name='rl_params',
            default_value='',
            description='Full path to robot_localization parameters file',
        ),
        DeclareLaunchArgument(
            name='gps',
            default_value='',
            description='Fuse GPS?',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )