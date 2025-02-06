# Copyright (c) 2018 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the 'License');
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an 'AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, GroupAction, SetEnvironmentVariable, OpaqueFunction
from launch.conditions import IfCondition, UnlessCondition
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import LoadComposableNodes
from launch_ros.actions import Node
from launch_ros.descriptions import ComposableNode, ParameterFile
from launch_ros.substitutions import FindPackageShare
from nav2_common.launch import RewrittenYaml

def launch_setup(context, *args, **kwargs):
    autostart = LaunchConfiguration('autostart')
    container_name = LaunchConfiguration('container_name')
    log_level = LaunchConfiguration('log_level')
    namespace = LaunchConfiguration('namespace')
    params_file = LaunchConfiguration('params_file')
    use_composition = LaunchConfiguration('use_composition')
    use_respawn = LaunchConfiguration('use_respawn')
    use_sim_time = LaunchConfiguration('use_sim_time')

    lifecycle_nodes = ['controller_server',
                       'smoother_server',
                       'planner_server',
                       'behavior_server',
                       'bt_navigator',
                       'waypoint_follower',
                       'velocity_smoother']

    # Map fully qualified names to relative ones so the node's namespace can be prepended.
    # In case of the transforms (tf), currently, there doesn't seem to be a better alternative
    # https://github.com/ros/geometry2/issues/32
    # https://github.com/ros/robot_state_publisher/pull/30
    # TODO(orduno) Substitute with `PushNodeRemapping`
    #              https://github.com/ros2/launch_ros/issues/56
    remappings = [('/tf', 'tf'),
                  ('/tf_static', 'tf_static')]

    # Create our own temporary YAML files that include substitutions
    param_substitutions = {
        'use_sim_time': use_sim_time,
        'autostart': autostart,
    }

    # sim_substitutions = {
    #     'max_vel_x': '20.0',
    #     'max_vel_theta': '20.0',
    #     'max_speed_xy': '20.0',
    #     'acc_lim_x': '20.0',
    #     'acc_lim_theta': '20.0',
    #     'decel_lim_x': '-0.35',
    #     'decel_lim_theta': '-0.35',
    #     'linear_granularity': '0.1',
    #     'angular_granularity': '0.1',
    #     'max_rotational_vel':'20.0',
    #     'min_rotational_vel':'0.1',
    #     'rotational_acc_lim':'20.0',
    # }

    in_sim = (use_sim_time.perform(context).lower() == 'true')

    configured_params = ParameterFile(
        RewrittenYaml(
            source_file=params_file,
            root_key=namespace,
            param_rewrites= {**param_substitutions}, #**(sim_substitutions if in_sim else {})},
            convert_types=True),
        allow_substs=True,
    )

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
                    parameters=[configured_params],
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
                    parameters=[configured_params],
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
                    parameters=[configured_params],
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
                    parameters=[configured_params],
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
                    parameters=[configured_params],
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
                    parameters=[configured_params],
                    arguments=['--ros-args', '--log-level', log_level],
                    remappings=remappings + [('cmd_vel', 'cmd_vel_nav'), ('cmd_vel_smoothed', 'cmd_vel')],
                ),
                # Node(
                #     package='nav2_collision_monitor',
                #     executable='collision_monitor',
                #     name='collision_monitor',
                #     output='screen',
                #     emulate_tty=True,  # https://github.com/ros2/launch/issues/188
                #     parameters=[configured_params],
                # ),
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
                    parameters=[configured_params],
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
                            parameters=[configured_params],
                            remappings=remappings + [('cmd_vel', 'cmd_vel_nav')],
                        ),
                        ComposableNode(
                            package='nav2_smoother',
                            plugin='nav2_smoother::SmootherServer',
                            name='smoother_server',
                            parameters=[configured_params],
                            remappings=remappings,
                        ),
                        ComposableNode(
                            package='nav2_planner',
                            plugin='nav2_planner::PlannerServer',
                            name='planner_server',
                            parameters=[configured_params],
                            remappings=remappings,
                        ),
                        ComposableNode(
                            package='nav2_behaviors',
                            plugin='behavior_server::BehaviorServer',
                            name='behavior_server',
                            parameters=[configured_params],
                            remappings=remappings,
                        ),
                        ComposableNode(
                            package='nav2_waypoint_follower',
                            plugin='nav2_waypoint_follower::WaypointFollower',
                            name='waypoint_follower',
                            parameters=[configured_params],
                            remappings=remappings,
                        ),
                        ComposableNode(
                            package='nav2_velocity_smoother',
                            plugin='nav2_velocity_smoother::VelocitySmoother',
                            name='velocity_smoother',
                            parameters=[configured_params],
                            remappings=remappings + [('cmd_vel', 'cmd_vel_nav'), ('cmd_vel_smoothed', 'cmd_vel')],
                        ),
                        # ComposableNode(
                        #     package='nav2_collision_monitor',
                        #     plugin='nav2_collision_monitor::CollisionMonitor',
                        #     name='collision_monitor',
                        #     parameters=[configured_params],
                        #     remappings=remappings +
                        #     [('cmd_vel', 'cmd_vel_nav')]
                        # ),
                        # ComposableNode(
                        #     package='nav2_map_server',
                        #     plugin='nav2_map_server::MapServer',
                        #     name='map_server',
                        #     parameters=[configured_params, {'yaml_filename': map_yaml_file}],
                        #     remappings=remappings + [('map', 'static_map')],
                        # ),
                        ComposableNode(
                            package='nav2_bt_navigator',
                            plugin='nav2_bt_navigator::BtNavigator',
                            name='bt_navigator',
                            parameters=[configured_params],
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
            description='the name of conatiner that nodes will load in if use composition',
        ),
        DeclareLaunchArgument(
            name='namespace',
            default_value='',
            description='Top-level namespace',
        ),
        DeclareLaunchArgument(
            name='log_level',
            default_value='info',
            description='log level',
        ),
        DeclareLaunchArgument(
            name='params_file',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'nav2.yaml']),
            description='Full path to the ROS2 parameters file to use for all launched nodes',
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
