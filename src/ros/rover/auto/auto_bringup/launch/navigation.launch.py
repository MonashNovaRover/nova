'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This launch file is responsible for navigation.
It launches our nav2 stack.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - nav2_controller/controller_server
  - nav2_smoother/smoother_server
  - nav2_planner/planner_server
  - nav2_behaviors/behavior_server
  - nav2_waypoint_follower/waypoint_follower
  - nav2_velocity_smoother/velocity_smoother
  - nav2_map_server/map_server
  - nav2_lifecycle_manager/lifecycle_manager
  - nav2_bt_navigator/bt_navigator
  - nova_utils/goal_marker.py
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	auto_bringup
CREATION:	UNKNOWN
EDITED:     05/01/2026
EDITED BY:  Anthony Lew, Terry Tian
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, GroupAction, SetEnvironmentVariable, OpaqueFunction
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, IfElseSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
import os

def launch_setup(context, *args, **kwargs):
    # package directories
    local = LaunchConfiguration('local')

    auto_bringup_dir = IfElseSubstitution(local,
        PathJoinSubstitution([os.path.expanduser("~") + '/nova/src/ros/rover/auto/auto_bringup']),
        FindPackageShare('auto_bringup')
    )

    comp = LaunchConfiguration('comp').perform(context).lower()
    
    # comp agnostic arguments
    autostart = LaunchConfiguration('autostart')
    log_level = LaunchConfiguration('log_level')
    map_params = LaunchConfiguration('map_params')
    ground_seg_params = LaunchConfiguration('ground_seg_params')
    namespace = LaunchConfiguration('namespace')
    publish_goals = LaunchConfiguration('publish_goals')
    use_respawn = LaunchConfiguration('use_respawn')
    gazebo = LaunchConfiguration('gazebo')
    mppi = LaunchConfiguration('mppi').perform(context).lower() == 'true'
    mppi_config = LaunchConfiguration('mppi_config').perform(context)

    # comp defaults
    if comp == 'arch':
        nav2_params_dir = PathJoinSubstitution([auto_bringup_dir, 'params', 'nav2_arch'])
    elif comp == 'urc':
        nav2_params_dir = PathJoinSubstitution([auto_bringup_dir, 'params', 'nav2_urc'])
    else:
        raise ValueError('"comp" arg must be either "arch" or "urc"')

    # Substitute params for each node with launch params
    substitution_params = {
        'use_sim_time': gazebo,
        'autostart': autostart,
    }
    # Combine all params from sim, substitution, and nav2 directory
    nav2_params = [PathJoinSubstitution([nav2_params_dir, params]) for params in os.listdir(nav2_params_dir.perform(context)) if params[-5:] == '.yaml']
    nav2_params.append(substitution_params)
    if mppi:
        mppi_params = PathJoinSubstitution([auto_bringup_dir, 'params', 'nav2_mppi', mppi_config + '.yaml'])
        if os.path.exists(mppi_params.perform(context)):
            nav2_params.append(mppi_params)
        else:
            raise ValueError(f'MPPI config "{mppi_config}" does not exist in auto_bringup/params/nav2_mppi/')

    lifecycle_nodes = ['controller_server',
                       'smoother_server',
                       'planner_server',
                       'behavior_server',
                       'bt_navigator',
                       'waypoint_follower',
                       'velocity_smoother',
                       'map_server']
    # Map fully qualified names to relative ones so the node's namespace can be prepended.
    # In case of the transforms (tf), currently, there doesn't seem to be a better alternative
    remappings = [('/tf', 'tf'),
                  ('/tf_static', 'tf_static')]
    
    return [
        Node(
            package="ground_segmentation_ros2",
            executable="ground_segmentation_ros2_node",
            parameters=[ground_seg_params, {"use_sim_time": gazebo}],
            remappings=[
                ("/ground_segmentation/input_pointcloud", '/livox/lidar_masked'),
                ("/ground_segmentation/input_imu", '/livox/imu'),
            ],
            output="screen",
        ),
        GroupAction(
            actions=[
                Node(
                    package='nav2_controller',
                    executable='controller_server',
                    output='screen',
                    respawn=use_respawn,
                    respawn_delay=2.0,
                    parameters=nav2_params,
                    arguments=['--ros-args', '--log-level', log_level],
                    remappings=remappings + [('cmd_vel', 'cmd_vel_nav')],
                ),
                Node(
                    package='nav2_smoother',
                    executable='smoother_server',
                    name='smoother_server',
                    output='screen',
                    respawn=use_respawn,
                    respawn_delay=2.0,
                    parameters=nav2_params,
                    arguments=['--ros-args', '--log-level', log_level],
                    remappings=remappings,
                ),
                Node(
                    package='nav2_planner',
                    executable='planner_server',
                    name='planner_server',
                    output='screen',
                    respawn=use_respawn,
                    respawn_delay=2.0,
                    parameters=nav2_params,
                    arguments=['--ros-args', '--log-level', log_level],
                    remappings=remappings,
                ),
                Node(
                    package='nav2_behaviors',
                    executable='behavior_server',
                    name='behavior_server',
                    output='screen',
                    respawn=use_respawn,
                    respawn_delay=2.0,
                    parameters=nav2_params,
                    arguments=['--ros-args', '--log-level', log_level],
                    remappings=remappings,
                ),
                Node(
                    package='nav2_waypoint_follower',
                    executable='waypoint_follower',
                    name='waypoint_follower',
                    output='screen',
                    respawn=use_respawn,
                    respawn_delay=2.0,
                    parameters=nav2_params,
                    arguments=['--ros-args', '--log-level', log_level],
                    remappings=remappings,
                ),
                Node(
                    package='nav2_velocity_smoother',
                    executable='velocity_smoother',
                    name='velocity_smoother',
                    output='screen',
                    respawn=use_respawn,
                    respawn_delay=2.0,
                    parameters=nav2_params,
                    arguments=['--ros-args', '--log-level', log_level],
                    remappings=remappings + [('cmd_vel', 'cmd_vel_nav'), ('cmd_vel_smoothed', 'cmd_vel')],
                ),
                # Node(
                #     package='nav2_collision_monitor',
                #     executable='collision_monitor',
                #     name='collision_monitor',
                #     output='screen',
                #     emulate_tty=True,  # https://github.com/ros2/launch/issues/188
                #     parameters=nav2_params,
                # ),
                Node(
                    package='nav2_map_server',
                    executable='map_server',
                    name='map_server',
                    parameters=nav2_params + [{'yaml_filename': map_params}],
                    remappings=remappings + [('map', 'static_map')],
                ),
                Node(
                    package='nav2_lifecycle_manager',
                    executable='lifecycle_manager',
                    name='lifecycle_manager_navigation',
                    output='screen',
                    arguments=['--ros-args', '--log-level', log_level],
                    parameters=[{'use_sim_time': gazebo},
                                {'autostart': autostart},
                                {'node_names': lifecycle_nodes}],
                ),
                Node(
                    package='nav2_bt_navigator',
                    executable='bt_navigator',
                    name='bt_navigator',
                    output='screen',
                    respawn=use_respawn,
                    respawn_delay=2.0,
                    parameters=nav2_params,
                    arguments=['--ros-abt_navigatorrgs', '--log-level', log_level],
                    remappings=remappings,
                )],
        ),
        SetEnvironmentVariable('RCUTILS_LOGGING_BUFFERED_STREAM', '1'),
        Node(
            condition=IfCondition(publish_goals),
            package='nova_utils',
            executable='goal_marker.py',
            namespace=namespace,
        ),
    ]


def generate_launch_description():
    local = LaunchConfiguration('local')

    auto_bringup_dir = IfElseSubstitution(local,
        PathJoinSubstitution([os.path.expanduser("~") + '/nova/src/ros/rover/auto/auto_bringup']),
        FindPackageShare('auto_bringup')
    )

    declared_arguments = [
        DeclareLaunchArgument(
            name='local',
            default_value='False',
            description='Whether to use local directories instead of the nix store.',
        ),
        DeclareLaunchArgument(
            name='comp',
            default_value='arch',
            description='ARCh or URC',
        ),
        # comp agnostic arguments
        DeclareLaunchArgument(
            name='autostart',
            default_value='True',
            description='Automatically startup the nav2 stack',
        ),
        DeclareLaunchArgument(
            name='log_level',
            default_value='info',
            description='log level',
        ),
        DeclareLaunchArgument(
            name='map_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'map.yaml']),
            description='Full path to the parameters file to use for static map layer',
        ),
        DeclareLaunchArgument(
            name='ground_seg_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'ground_segmentation.yaml']),
            description='Full path to the parameters file to use for ground segmentation',
        ),
        DeclareLaunchArgument(
            name='namespace',
            default_value='',
            description='Top-level namespace',
        ),
        DeclareLaunchArgument(
            name='publish_goals',
            default_value='True',
            description='Publish Nav2 goals as a MarkerArray?',
        ),
        DeclareLaunchArgument(
            name='use_respawn',
            default_value='False',
            description='Whether to respawn if a node crashes. Applied when composition is disabled.',
        ),
        DeclareLaunchArgument(
            name='gazebo',
            default_value='False',
            description='Use simulation (Gazebo) clock if True',
        ),
        DeclareLaunchArgument(
            name='mppi',
            default_value='True',
            description='Using MPPI?', 
        ),
        DeclareLaunchArgument(
            name='mppi_config',
            default_value='regular',
            description='Name of the MPPI config to use (without .yaml)', 
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
