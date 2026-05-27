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
from launch.actions import DeclareLaunchArgument, OpaqueFunction, GroupAction, IncludeLaunchDescription
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, IfElseSubstitution, AndSubstitution, NotSubstitution, EnvironmentVariable
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

from os.path import expanduser

def launch_setup(context, *args, **kwargs):
    # package directories
    local = LaunchConfiguration('local')
    
    auto_bringup_dir = IfElseSubstitution(local,
        PathJoinSubstitution([expanduser("~") + '/nova/src/ros/rover/auto/auto_bringup']),
        FindPackageShare('auto_bringup')
    )
    nova_bringup_dir = IfElseSubstitution(local,
        PathJoinSubstitution([expanduser("~") + '/nova/src/ros/rover/nova_bringup']),
        FindPackageShare('nova_bringup')
    )

    comp = LaunchConfiguration('comp').perform(context).lower()

    # comp agnostic arguments
    sim = LaunchConfiguration('sim')

    # comp defaults
    if comp == 'arch':
        rl_params = PathJoinSubstitution([auto_bringup_dir, 'params', 'arch', 'rl_arch.yaml'])
        cartographer = 'False'
    elif comp == 'urc':
        rl_params = PathJoinSubstitution([auto_bringup_dir, 'params', 'urc', 'rl_urc.yaml'])
        cartographer = 'True'
    else:
        raise ValueError('Invalid comp value')
    
    # comp defaults overrides
    if LaunchConfiguration('rl_params').perform(context) != '':
        rl_params = LaunchConfiguration('rl_params')
    if LaunchConfiguration('cartographer').perform(context) != '':
        cartographer = LaunchConfiguration('cartographer')

    return [
        Node(
            condition=IfCondition(str(comp == 'arch')),
            package='robot_localization',
            executable='ekf_node',
            name='ekf_filter_node',
            output='screen',
            parameters=[rl_params, {'use_sim_time': sim}],
        ),
        GroupAction(
            condition=IfCondition(str(comp == 'urc')),
            actions=[
                Node(
                    package='robot_localization',
                    executable='ekf_node',
                    name='ekf_filter_node_odom',
                    output='screen',
                    parameters=[rl_params, {'use_sim_time': sim}],
                ),
                Node(
                    package='robot_localization',
                    executable='ekf_node',
                    name='ekf_filter_node_map',
                    output='screen',
                    parameters=[rl_params, {'use_sim_time': sim}],
                    remappings=[('odometry/filtered', 'odometry/global')],
                ),
                # Why are there more nodes for GPS?
                # https://docs.ros.org/en/api/robot_localization/html/integrating_gps.html
                Node(
                    package='robot_localization',
                    executable='navsat_transform_node',
                    name='navsat_transform',
                    output='screen',
                    parameters=[rl_params, {'use_sim_time': sim}],
                    remappings=[('odometry/filtered', 'odometry/global'),
                                ('gps/fix', 'gps_rover/fix'),
                                ('imu', 'gps_rover/heading_imu')],
                ),
                GroupAction(
                    condition=IfCondition(AndSubstitution(cartographer, NotSubstitution(sim))),
                    actions=[
                        IncludeLaunchDescription(
                            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([nova_bringup_dir, 'launch', 'gps_rover.launch.py'])),
                            launch_arguments={'publish_fix_custom': 'False'}.items(),
                        ),
                        Node(
                            package='electronics',
                            namespace='',
                            executable='magnetometer.py',
                            name='magnetometer',
                        ),
                        Node(
                            package='nova_utils',
                            namespace='',
                            executable='ekf_to_cartographer.py',
                            name='ekf_to_cartographer',
                        ),
                    ],
                ),
            ],
        ),
    ]

def generate_launch_description():
    declared_arguments = [
        DeclareLaunchArgument(
            name='local',
            default_value='False',
            description='Whether to use local directories instead of the nix store.',
        ),
        DeclareLaunchArgument(
            name='comp',
            default_value=EnvironmentVariable('COMP', default_value='ARCh'),
            description='ARCh or URC',
        ),
        # comp agnostic arguments
        DeclareLaunchArgument(
            name='sim',
            default_value='False',
            description='Use simulation clock if True',
        ),
        # arguments with comp defaults
        DeclareLaunchArgument(
            name='rl_params',
            default_value='',
            description='Full path to robot_localization parameters file',
        ),
        DeclareLaunchArgument(
            name='cartographer',
            default_value='',
            description='For use with cartographer?',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
