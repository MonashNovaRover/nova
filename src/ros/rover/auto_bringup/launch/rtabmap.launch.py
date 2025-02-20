import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.descriptions import ComposableNode
from launch_ros.substitutions import FindPackageShare
from launch_ros.actions import ComposableNodeContainer

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

    use_sim_time = LaunchConfiguration('use_sim_time')
    name = LaunchConfiguration('name').perform(context)
    x = LaunchConfiguration('x').perform(context)
    y = LaunchConfiguration('y').perform(context)
    z = LaunchConfiguration('z').perform(context)
    roll = LaunchConfiguration('roll').perform(context)
    pitch = LaunchConfiguration('pitch').perform(context)
    yaw = LaunchConfiguration('yaw').perform(context)
    camera = LaunchConfiguration('camera').perform(context)
    rtabmap_viz = LaunchConfiguration('rtabmap_viz').perform(context)
    rtabmap_params = LaunchConfiguration('rtabmap_params').perform(context)

    remappings = [
        ('rgb/image', name+'/rgb/image_raw'),
        ('rgb/camera_info', name+'/rgb/camera_info'),
        ('depth/image', name+'/stereo/image_raw'),
        ('rgbd_image',name+'/rgbd/image_raw'),
        ('rgbd_image/compressed',name+'/rgbd/image_raw_compressed'),
        ('cloud',name+'/depth/points'),
        ('obstacles', name+'/obstacles'),
        ('ground', name+'/ground'),
        # ('imu', name+'/imu/data'),
        ('odom', 'odom/visual'),
    ]

    return [
        OpaqueFunction(function=delete_rtabmap_db),
        IncludeLaunchDescription(
            condition=IfCondition(camera),
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'camera.launch.py'])),
            launch_arguments={
                'pointclouds':'False',
                'gazebo': use_sim_time,
            }.items()
        ),
        ComposableNodeContainer(
            name=f'{name}_mapping_container',
            namespace='',
            package='rclcpp_components',
            executable='component_container',
            composable_node_descriptions=[
                ComposableNode(
                    package='rtabmap_sync',
                    plugin='rtabmap_sync::RGBDSync',
                    name='rtabmap_sync',
                    parameters=[rtabmap_params],
                    remappings=remappings,
                ),
                ComposableNode(
                    package='rtabmap_odom',
                    plugin='rtabmap_odom::RGBDOdometry',
                    name='rtabmap_odom',
                    parameters=[rtabmap_params, {'initial_pose': f'{x} {y} {z} {roll} {pitch} {yaw}', 'use_sim_time': use_sim_time}],
                    remappings=remappings,
                ),
                ComposableNode(
                    package='rtabmap_slam',
                    plugin='rtabmap_slam::CoreWrapper',
                    name='rtabmap_slam',
                    parameters=[rtabmap_params, {'use_sim_time': use_sim_time, 'rtabmap_args': '--delete_db_on_start'}],
                    remappings=remappings,
                ),
            ],
        ),
        Node(
            package='rtabmap_util', executable='obstacles_detection', output='screen',
            parameters=[rtabmap_params],
            remappings=remappings,
        ),
        Node(
            condition=IfCondition(rtabmap_viz),
            package='rtabmap_viz',
            executable='rtabmap_viz',
            output='screen',
            parameters=[rtabmap_params, {'use_sim_time': use_sim_time}],
            remappings=remappings,
        ),
    ]


def generate_launch_description():
    auto_bringup_dir = FindPackageShare('auto_bringup')

    declared_arguments = [
        DeclareLaunchArgument(name='name', default_value='oak'),
        DeclareLaunchArgument(name='rtabmap_viz', default_value='False'),
        DeclareLaunchArgument(name='use_sim_time', default_value='False'), # Revert ASAP
        DeclareLaunchArgument(name='camera', default_value='False'),
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
