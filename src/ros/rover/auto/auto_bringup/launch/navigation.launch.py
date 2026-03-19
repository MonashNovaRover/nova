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
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
import os

def launch_setup(context, *args, **kwargs):
    # package directories
    auto_bringup_dir = FindPackageShare('auto_bringup')

    comp = LaunchConfiguration('comp').perform(context).lower()
    
    # comp agnostic arguments
    autostart = LaunchConfiguration('autostart')
    log_level = LaunchConfiguration('log_level')
    map_params = LaunchConfiguration('map_params')
    namespace = LaunchConfiguration('namespace')
    sim_params = LaunchConfiguration('sim_params')
    publish_goals = LaunchConfiguration('publish_goals')
    use_respawn = LaunchConfiguration('use_respawn')
    gazebo = LaunchConfiguration('gazebo')
    mppi = LaunchConfiguration('mppi').perform(context).lower() == 'true'
    mppi_config = LaunchConfiguration('mppi_config').perform(context)
    obstacles_detection = LaunchConfiguration('obstacles_detection')

    use_lidar = LaunchConfiguration('obstacles_detection').perform(context).lower() == 'true'

    if use_lidar:
        pc_input = '/livox/lidar_masked'
        pc_downsampled = '/livox/lidar_downsampled'
        pc_obstacles = '/livox/lidar/obstacles'
        pc_ground = '/livox/lidar/ground'
    else:
        pc_input = '/d415/depth/color/points'
        pc_downsampled = '/d415/depth/downsampled'
        pc_obstacles = '/d415/obstacles'
        pc_ground = '/d415/ground'

    # comp defaults
    if comp == 'arch':
        nav2_params_dir = PathJoinSubstitution([auto_bringup_dir, 'params', 'nav2_arch'])
    elif comp == 'urc':
        nav2_params_dir = PathJoinSubstitution([auto_bringup_dir, 'params', 'nav2_urc'])
    else:
        raise ValueError('"comp" arg must be either "arch" or "urc"')

    # comp defaults overrides
    if LaunchConfiguration('nav2_params_dir').perform(context) != '':
        nav2_params_dir = LaunchConfiguration('nav2_params_dir')

    in_sim = (gazebo.perform(context).lower() == 'true')
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
        GroupAction(
            condition=IfCondition(obstacles_detection),
            actions = [
                Node(
                    package='pcl_ros',
                    executable='filter_voxel_grid_node',
                    name='voxel_grid_filter',
                    parameters=[{'leaf_size': 0.05}],
                    remappings=[('input', pc_input),
                                ('output', pc_downsampled)],
                ),
                Node(
                    package='rtabmap_util',
                    executable='obstacles_detection',
                    name='rtabmap_obstacles_detection',
                    output='screen',
                    parameters=[{'max_obstacle_height': 1.6,
                                 'ground_normal_angle': 1.25664}], # ~72 degrees in radians
                    remappings=[
                        ('cloud',pc_downsampled),
                        ('obstacles',pc_obstacles),
                        ('ground', pc_ground),
                    ],
                ),
            ],
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
    auto_bringup_dir = FindPackageShare('auto_bringup')

    declared_arguments = [
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
            name='sim_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'nav2_sim.yaml']),
            description='Sim parameters to use if using sim time', 
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
        DeclareLaunchArgument(
            name='obstacles_detection',
            default_value='True',
            description='Run RTAB-Map node for obstacle detection?',
        ),
        # arguments with comp defaults
        DeclareLaunchArgument(
            name='nav2_params_dir',
            default_value='',
            description='Full path to the folder with ROS2 parameters files to use with all nodes',
        ),
        DeclareLaunchArgument(
            name='obstacles_detection',
            default_value='True',
            description='Run RTAB-Map node for obstacle detection?',
        ),
        DeclareLaunchArgument(
            name='lidar',
            default_value='False',
            description='Use Livox lidar pointcloud for obstacle detection (default False uses Realsense d415)',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
