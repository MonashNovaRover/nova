'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This launch file is responsible for navigation.
It launches our nav2 stack.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  -
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	auto_bringup
CREATION:	UNKNOWN
EDITED:     24/04/2025
EDITED BY:  Anthony Lew
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, GroupAction, SetEnvironmentVariable, OpaqueFunction
from launch.conditions import IfCondition, UnlessCondition
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, PythonExpression
from launch_ros.actions import LoadComposableNodes
from launch_ros.actions import Node
from launch_ros.descriptions import ComposableNode, ParameterFile
from launch_ros.substitutions import FindPackageShare
from nav2_common.launch import RewrittenYaml

def launch_setup(context, *args, **kwargs):
    auto_bringup_dir = FindPackageShare('auto_bringup')

    autostart = LaunchConfiguration('autostart')
    container_name = LaunchConfiguration('container_name')
    log_level = LaunchConfiguration('log_level')
    map_params = LaunchConfiguration('map_params')
    namespace = LaunchConfiguration('namespace')
    nav2_params = LaunchConfiguration('nav2_params')
    publish_goals = LaunchConfiguration('publish_goals')
    use_composition = LaunchConfiguration('use_composition')
    use_respawn = LaunchConfiguration('use_respawn')
    use_sim_time = LaunchConfiguration('use_sim_time')

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
    # https://github.com/ros/geometry2/issues/32
    # https://github.com/ros/robot_state_publisher/pull/30
    # TODO(orduno) Substitute with `PushNodeRemapping`
    #              https://github.com/ros2/launch_ros/issues/56
    remappings = [('/tf', 'tf'),
                  ('/tf_static', 'tf_static')]

    # Create our own temporary YAML files that include substitutions
    substitution_params = {
        'use_sim_time': use_sim_time,
        'autostart': autostart,
    }

    sim_params = PathJoinSubstitution([auto_bringup_dir, 'params', 'nav2_sim.yaml'])
    
    in_sim = (use_sim_time.perform(context).lower() == 'true')

    return [
        GroupAction(
            condition=UnlessCondition(use_composition),
            actions=[
                Node(
                    package='nav2_controller',
                    executable='controller_server',
                    output='screen',
                    respawn=use_respawn,
                    respawn_delay=2.0,
                    parameters=[nav2_params, substitution_params, sim_params if in_sim else {}],
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
                    parameters=[nav2_params, substitution_params, sim_params if in_sim else {}],
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
                    parameters=[nav2_params, substitution_params, sim_params if in_sim else {}],
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
                    parameters=[nav2_params, substitution_params, sim_params if in_sim else {}],
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
                    parameters=[nav2_params, substitution_params, sim_params if in_sim else {}],
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
                    parameters=[nav2_params, substitution_params, sim_params if in_sim else {}],
                    arguments=['--ros-args', '--log-level', log_level],
                    remappings=remappings + [('cmd_vel', 'cmd_vel_nav'), ('cmd_vel_smoothed', 'cmd_vel')],
                ),
                # Node(
                #     package='nav2_collision_monitor',
                #     executable='collision_monitor',
                #     name='collision_monitor',
                #     output='screen',
                #     emulate_tty=True,  # https://github.com/ros2/launch/issues/188
                #     parameters=[nav2_params, substitution_params, sim_params if in_sim else {}],
                # ),
                Node(
                    package='nav2_map_server',
                    executable='map_server',
                    name='map_server',
                    parameters=[nav2_params, substitution_params, sim_params if in_sim else {}, {'yaml_filename': map_params}],
                    remappings=remappings + [('map', 'static_map')],
                ),
                Node(
                    package='nav2_lifecycle_manager',
                    executable='lifecycle_manager',
                    name='lifecycle_manager_navigation',
                    output='screen',
                    arguments=['--ros-args', '--log-level', log_level],
                    parameters=[{'use_sim_time': use_sim_time},
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
                    parameters=[nav2_params, substitution_params, sim_params if in_sim else {}],
                    arguments=['--ros-abt_navigatorrgs', '--log-level', log_level],
                    remappings=remappings,
                )],
        ),
        GroupAction(
            condition=IfCondition(use_composition),
            actions=[
                LoadComposableNodes(
                    target_container=(namespace, '/', container_name),
                    composable_node_descriptions=[
                        ComposableNode(
                            package='nav2_controller',
                            plugin='nav2_controller::ControllerServer',
                            name='controller_server',
                            parameters=[nav2_params, substitution_params, sim_params if in_sim else {}],
                            remappings=remappings + [('cmd_vel', 'cmd_vel_nav')],
                        ),
                        ComposableNode(
                            package='nav2_smoother',
                            plugin='nav2_smoother::SmootherServer',
                            name='smoother_server',
                            parameters=[nav2_params, substitution_params, sim_params if in_sim else {}],
                            remappings=remappings,
                        ),
                        ComposableNode(
                            package='nav2_planner',
                            plugin='nav2_planner::PlannerServer',
                            name='planner_server',
                            parameters=[nav2_params, substitution_params, sim_params if in_sim else {}],
                            remappings=remappings,
                        ),
                        ComposableNode(
                            package='nav2_behaviors',
                            plugin='behavior_server::BehaviorServer',
                            name='behavior_server',
                            parameters=[nav2_params, substitution_params, sim_params if in_sim else {}],
                            remappings=remappings,
                        ),
                        ComposableNode(
                            package='nav2_waypoint_follower',
                            plugin='nav2_waypoint_follower::WaypointFollower',
                            name='waypoint_follower',
                            parameters=[nav2_params, substitution_params, sim_params if in_sim else {}],
                            remappings=remappings,
                        ),
                        ComposableNode(
                            package='nav2_velocity_smoother',
                            plugin='nav2_velocity_smoother::VelocitySmoother',
                            name='velocity_smoother',
                            parameters=[nav2_params, substitution_params, sim_params if in_sim else {}],
                            remappings=remappings + [('cmd_vel', 'cmd_vel_nav'), ('cmd_vel_smoothed', 'cmd_vel')],
                        ),
                        # ComposableNode(
                        #     package='nav2_collision_monitor',
                        #     plugin='nav2_collision_monitor::CollisionMonitor',
                        #     name='collision_monitor',
                        #     parameters=[nav2_params, substitution_params, sim_params if in_sim else {}],
                        #     remappings=remappings +
                        #     [('cmd_vel', 'cmd_vel_nav')]
                        # ),
                        ComposableNode(
                            package='nav2_map_server',
                            plugin='nav2_map_server::MapServer',
                            name='map_server',
                            parameters=[nav2_params, substitution_params, sim_params if in_sim else {}, {'yaml_filename': map_params}],
                            remappings=remappings + [('map', 'static_map')],
                        ),
                        ComposableNode(
                            package='nav2_bt_navigator',
                            plugin='nav2_bt_navigator::BtNavigator',
                            name='bt_navigator',
                            parameters=[nav2_params, substitution_params, sim_params if in_sim else {}],
                            remappings=remappings,
                        ),
                        ComposableNode(
                            package='nav2_lifecycle_manager',
                            plugin='nav2_lifecycle_manager::LifecycleManager',
                            name='lifecycle_manager_navigation',
                            parameters=[{'use_sim_time': use_sim_time,
                                         'autostart': autostart,
                                         'node_names': lifecycle_nodes}],
                        )])],
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
            name='autostart',
            default_value='True',
            description='Automatically startup the nav2 stack',
        ),
        DeclareLaunchArgument(
            name='container_name',
            default_value='nav2_container',
            description='the name of container that nodes will load in if use composition',
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
            name='nav2_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'nav2_urc.yaml']),
            description='Full path to the ROS2 parameters file to use for all launched Nav2 nodes',
        ),
        DeclareLaunchArgument(
            name='publish_goals',
            default_value='True',
            description='Publish Nav2 goals as a MarkerArray?',
        ),
        DeclareLaunchArgument(
            name='use_composition',
            default_value='False',
            description='Use composed bringup if True',
        ),
        DeclareLaunchArgument(
            name='use_respawn',
            default_value='False',
            description='Whether to respawn if a node crashes. Applied when composition is disabled.',
        ),
        DeclareLaunchArgument(
            name='use_sim_time',
            default_value='False',
            description='Use simulation (Gazebo) clock if True',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
