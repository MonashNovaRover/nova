import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction, RegisterEventHandler
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.descriptions import ComposableNode
from launch_ros.substitutions import FindPackageShare
from launch_ros.actions import ComposableNodeContainer
from launch.event_handlers import OnProcessExit

# This function does what --delete_db_on_start
# Because we are launching it as a ComposableNode, we can't pass arguments to the executable
# Hence we delete the db file here for it to not mess with us later
def delete_rtabmap_db(context, *args, **kwargs):
    file_path = os.path.expanduser('~/.ros/rtabmap.db')
    try:
        if os.path.exists(file_path):
            os.remove(file_path)
            print(f'Deleted rtabmap.db at {file_path}')
        else:
            print(f'rtabmap.db not found at {file_path}. You might have to delete the file manually, wherever it is')
    except Exception as e:
        print(f'Failed to delete file {file_path}: {e}')

def launch_setup(context, *args, **kwargs):
    auto_bringup_dir = FindPackageShare('auto_bringup')

    gazebo = LaunchConfiguration('gazebo')
    front_name = LaunchConfiguration('front_name').perform(context)
    back_name = LaunchConfiguration('back_name').perform(context)
    x = LaunchConfiguration('x').perform(context)
    y = LaunchConfiguration('y').perform(context)
    z = LaunchConfiguration('z').perform(context)
    roll = LaunchConfiguration('roll').perform(context)
    pitch = LaunchConfiguration('pitch').perform(context)
    yaw = LaunchConfiguration('yaw').perform(context)
    rtabmap_viz = LaunchConfiguration('rtabmap_viz').perform(context)
    rtabmap_params = LaunchConfiguration('rtabmap_params').perform(context)

    nodes = [
        OpaqueFunction(function=delete_rtabmap_db),
        ComposableNodeContainer(
            name='rtabmap_mapping_container',
            namespace='',
            package='rclcpp_components',
            executable='component_container',
            composable_node_descriptions=[
                ComposableNode(
                    package='rtabmap_sync',
                    plugin='rtabmap_sync::RGBDSync',
                    name=f'{front_name}_rgbd_sync',
                    parameters=[rtabmap_params],
                    remappings=[
                        ('rgb/image', front_name+'/rgb/image_raw'),
                        ('rgb/camera_info', front_name+'/rgb/camera_info'),
                        ('depth/image', front_name+'/stereo/image_raw'),
                        ('rgbd_image',front_name+'/rgbd/image_raw')
                    ],
                ),
                # ComposableNode(
                #     package='rtabmap_sync',
                #     plugin='rtabmap_sync::RGBDSync',
                #     name=f'{back_name}_rgbd_sync',
                #     parameters=[rtabmap_params],
                #     remappings=[
                #         ('rgb/image', back_name+'/rgb/image_raw'),
                #         ('rgb/camera_info', back_name+'/rgb/camera_info'),
                #         ('depth/image', back_name+'/stereo/image_raw'),
                #         ('rgbd_image',back_name+'/rgbd/image_raw'),
                #     ],
                # ),
                ComposableNode(
                    package='rtabmap_odom',
                    plugin='rtabmap_odom::RGBDOdometry',
                    name='rtabmap_odom',
                    parameters=[rtabmap_params, {'initial_pose': f'{x} {y} {z} {roll} {pitch} {yaw}', 'use_sim_time': gazebo}],
                    remappings=[
                        ('odom', 'odom/visual'),
                        ('rgbd_image',front_name+'/rgbd/image_raw'),
                    ],
                ),
                ComposableNode(
                    package='rtabmap_slam',
                    plugin='rtabmap_slam::CoreWrapper',
                    name='rtabmap_slam',
                    parameters=[rtabmap_params, {'use_sim_time': gazebo, 'rtabmap_args': '--delete_db_on_start'}],
                    remappings=[
                        ('scan_cloud','/livox/lidar'),
                        ('rgbd_image','/oak/rgbd/image_raw'),
                        ('rgbd/camera_info','/oak/rgbd/camera_info'),
                        ('gps/fix','/gps_rover/fix')
                    ],
                ),
            ],
        ),
         Node(
             package='rtabmap_util', executable='obstacles_detection', output='screen',
             parameters=[rtabmap_params],
             remappings=[
                 ('cloud','/livox/lidar'),
                 ('obstacles','/livox/lidar/obstacles'),
                 ('ground', '/livox/lidar/ground'),
             ],
         ),
        Node(
            condition=IfCondition(rtabmap_viz),
            package='rtabmap_viz',
            executable='rtabmap_viz',
            output='screen',
            parameters=[rtabmap_params, {'use_sim_time': True}],
            remappings=[
                ('scan_cloud','/livox/lidar'),
                ('rgb/image','/oak/rgb/image_raw'),
                ('rgb/camera_info','/oak/rgb/camera_info')
            ]
        ),
    ]

    wait_for_topics = Node(
        package='nova_utils',
        executable='topic_waiter.py',
        name='rtabmap_wait_for_topics',
        output='screen',
        parameters=[
            {'topics': [
                f'/{front_name}/rgb/image_raw',
                f'/{front_name}/rgb/camera_info',
                f'/{front_name}/stereo/image_raw',
                '/livox/lidar',
                ]
            }
        ],
    )

    return [
        wait_for_topics,
        RegisterEventHandler(
            event_handler=OnProcessExit(
                target_action=wait_for_topics,
                on_exit=nodes,
            )
        ),
    ]


def generate_launch_description():
    auto_bringup_dir = FindPackageShare('auto_bringup')

    declared_arguments = [
        DeclareLaunchArgument(name='front_name', default_value='oak'),
        DeclareLaunchArgument(name='back_name', default_value='bootie'),
        DeclareLaunchArgument(name='rtabmap_viz', default_value='False'),
        DeclareLaunchArgument(name='gazebo', default_value='False'),
        DeclareLaunchArgument(name='x', default_value='0.0'),
        DeclareLaunchArgument(name='y', default_value='0.0'),
        DeclareLaunchArgument(name='z', default_value='0.0'),
        DeclareLaunchArgument(name='roll', default_value='0.0'),
        DeclareLaunchArgument(name='pitch', default_value='0.0'),
        DeclareLaunchArgument(name='yaw', default_value='0.0'),
        DeclareLaunchArgument(
            name='rtabmap_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'rtabmap.yaml']),
            description='Params file for RTABMap Nodes',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
