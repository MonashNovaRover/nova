'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This launch file is responsible for LiDAR.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - livox_lidar_publisher
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	auto_bringup
CREATION:	15/01/2026
EDITED:     15/01/2026
EDITED BY:  Kabilan Velmurugan Sujatha, Bailey 
    Chessum, Victor Bartlinski, Terry Tian
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from pathlib import Path
import zipfile
import subprocess
from logging import Logger

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction, GroupAction, RegisterEventHandler
from launch.event_handlers import OnProcessExit
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch.conditions import IfCondition, UnlessCondition
from launch.logging import get_logger
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


class Colour:
    RED = '\033[1;31m'
    GREEN = '\033[1;32m'
    YELLOW = '\033[0;33m'
    END = '\033[0m'

# Similar to RTABMap, we want to delete previous maps when running FAST-LIVO2
# for minimised disk space and convenient use. 
def delete_pcds(context, output_dir, logger):
    # Delete existing .pcd's in FAST-LIVO2 save directory.
    dir_path = Path(f'~/.ros/{output_dir}/pcd').expanduser()
    if dir_path.exists() and dir_path.is_dir():
        for item in dir_path.iterdir():
            if item.is_file():
                item.unlink()
            else:
                logger.warning(f"{Colour.YELLOW}{item} is not a file and was not deleted.{Colour.END}")
        logger.info(f"{Colour.GREEN}Directory {dir_path}/ cleared.{Colour.END}")
    else:
        # normal for directory to not exist on first run
        logger.error(f"{Colour.YELLOW}Directory {dir_path} does not exist.{Colour.END}")

def concat_pcds(context, output_dir, save_dir, logger):
    # Concatenate .pcd's in FAST-LIVO2 save directory into single .pcd.
    dir_path = Path(f'~/.ros/{output_dir}/pcd').expanduser()
    save_dir = Path(save_dir).expanduser()
    if dir_path.exists() and dir_path.is_dir():
        subprocess.run([
            "/bin/sh",
            "-c",
            f"cd {save_dir} && pcl_concatenate_points_pcd {dir_path}/*"
        ])
        logger.info(f"{Colour.GREEN}Map saved to {save_dir}/output.pcd.{Colour.END}")
        try:
            with zipfile.ZipFile(save_dir / 'output.pcd.zip', 'w', zipfile.ZIP_DEFLATED) as zipf:
                zipf.write(save_dir / 'output.pcd', arcname='output.pcd')
        except Exception as e:
            logger.error(f"{Colour.RED}Failed to zip the map: {e}{Colour.END}")
        logger.info(f"{Colour.GREEN}Map zipped to {save_dir}/output.pcd.zip.{Colour.END}")
    else:
        logger.error(f"{Colour.RED}Directory {dir_path} does not exist and could not be concatenated.{Colour.END}")

def block_until_enter_pressed(context, logger):
    logger.info(f"{Colour.YELLOW}Press Enter to start FAST-LIVO2 mapping...{Colour.END}")
    input()

def launch_setup(context, *args, **kwargs):
    auto_bringup_dir = FindPackageShare('auto_bringup')

    img_en = int(LaunchConfiguration('img_en').perform(context).lower() == 'true')
    img_topic = LaunchConfiguration('img_topic').perform(context)
    fastlivo2 = LaunchConfiguration('fastlivo2')
    fastlivo2_params = LaunchConfiguration('fastlivo2_params')
    obstacles_detection = LaunchConfiguration('obstacles_detection')
    output_dir = LaunchConfiguration('output_dir').perform(context)
    save_dir = LaunchConfiguration('save_dir').perform(context)
    lidar_config = LaunchConfiguration('lidar_config').perform(context)
    lidar_params = LaunchConfiguration('lidar_params')
    tfs = LaunchConfiguration('tfs')
    sim = LaunchConfiguration('sim')
    uncompress_img = LaunchConfiguration('uncompress_img')

    intrinsics_params = PathJoinSubstitution([auto_bringup_dir,'params','fast_livo2','d415_intrinsics.yaml'])
    extrinsics_params = PathJoinSubstitution([auto_bringup_dir,'params','fast_livo2','d415_extrinsics.yaml'])
    logger = get_logger("lidar_launch")

    fastlivo2_node = Node(
        package='fast_livo',
        executable='fastlivo_mapping',
        name='fastlivo2',
        output='screen',
        parameters=[fastlivo2_params, extrinsics_params,
                    {'use_sim_time': sim,
                     'save_folder': output_dir,
                     'img_en': img_en,
                     'img_topic': img_topic}],
    )

    wait_for_parameter_blackboard = Node(
        package='nova_utils',
        executable='node_waiter.py',
        name='fastlivo2_wait_for_parameter_blackboard',
        output='screen',
        parameters=[
            {'nodes': ['parameter_blackboard']}
        ],
    )

    wait_topics = ['/livox/lidar_masked', 'livox/imu']
    if img_en:
        wait_topics.append(img_topic)
    
    wait_for_topics = Node(
        package='nova_utils',
        executable='topic_waiter.py',
        name='fastlivo2_wait_for_topics',
        output='screen',
        parameters=[{'topics': wait_topics}],
    )

    return [
        Node(
            condition=UnlessCondition(sim),
            package='livox_ros_driver2',
            executable='livox_ros_driver2_node',
            name='livox_lidar_publisher',
            output='screen',
            parameters=[lidar_params, {'user_config_path': lidar_config, 'use_sim_time': sim}],
        ),
        Node(
            # Remove points that intersect with the rover
            package='pcl_ros',
            executable='filter_crop_box_node',
            name='crop_box_filter',
            parameters=[{'min_x': -0.64, 'max_x': 0.64,
                            'min_y': -0.57, 'max_y': 0.57,
                            'min_z': 0.0, 'max_z': 4.0,
                            'negative': True,
                            'input_frame': 'base_link'}],
            remappings=[('input', '/livox/lidar'),
                        ('output', '/livox/lidar_masked')],
        ),
        Node(
            # NOTE image_transport only creates subscribers if subscribers exist for its publishers. 
            condition=IfCondition(uncompress_img),
            package="image_transport",
            executable="republish",
            name="republish",
            output="screen",
            parameters=[{'in_transport': 'compressed', 
                         'out_transport': 'raw'}],
            remappings=[("in/compressed",  f"{img_topic}/compressed"), 
                        ("out", img_topic)],
        ),
        GroupAction(
            condition=IfCondition(fastlivo2),
            actions=[
                OpaqueFunction(
                    function=delete_pcds,
                    kwargs={'output_dir': output_dir,
                            'logger': logger}
                ),
                Node(
                    package='demo_nodes_cpp',
                    executable='parameter_blackboard',
                    name='parameter_blackboard',
                    parameters=[intrinsics_params, {'use_sim_time': sim}],
                    output='screen'
                ),
                wait_for_parameter_blackboard,
                RegisterEventHandler(
                    event_handler=OnProcessExit(
                        target_action=wait_for_parameter_blackboard,
                        on_exit=wait_for_topics,
                    ),
                ),
                RegisterEventHandler(
                    event_handler=OnProcessExit(
                        target_action=wait_for_topics,
                        on_exit=[OpaqueFunction(function=block_until_enter_pressed, kwargs={'logger': logger}),
                                 fastlivo2_node],
                    ),
                ),
                RegisterEventHandler(
                    event_handler=OnProcessExit(
                        target_action=fastlivo2_node,
                        on_exit=OpaqueFunction(
                            function=concat_pcds,
                            kwargs={'output_dir': output_dir, 
                                    'save_dir': save_dir,
                                    'logger': logger}
                        ),
                    ),
                ),
            ],
        ),
        GroupAction(
            condition=IfCondition(obstacles_detection),
            actions = [
                Node(
                    package='pcl_ros',
                    executable='filter_voxel_grid_node',
                    name='voxel_grid_filter',
                    parameters=[{'leaf_size': 0.05}],
                    remappings=[('input', '/livox/lidar_masked'),
                                ('output', '/livox/lidar_downsampled')],
                ),
                Node(
                    package='rtabmap_util',
                    executable='obstacles_detection',
                    name='rtabmap_obstacles_detection',
                    output='screen',
                    parameters=[{'max_obstacle_height': 1.6,
                                 'ground_normal_angle': 1.25664}], # ~72 degrees in radians
                    remappings=[
                        ('cloud','/livox/lidar_downsampled'),
                        ('obstacles','/livox/lidar/obstacles'),
                        ('ground', '/livox/lidar/ground'),
                    ],
                ),
            ],
        ),
        GroupAction(
            condition = IfCondition(tfs),
            actions = [
                Node(
                    package='tf2_ros',
                    executable='static_transform_publisher',
                    name='map_to_odom_static_tf_publisher',
                    arguments=["0", "0", "0", "0", "0", "0", "map", "odom"],
                    output='screen',
                ),
                Node(
                    package='tf2_ros',
                    executable='static_transform_publisher',
                    name='odom_to_camera_init_publisher',
                    # tf2_echo base_link to livox_frame
                    arguments=["0.541", "0", "0.950", "0", "0", "0", "odom", "camera_init"],
                    output='screen',
                ),
                Node(
                    package='tf2_ros',
                    executable='static_transform_publisher',
                    name='aft_mapped_to_base_link_publisher',
                    # tf2_echo livox_frame to base_link
                    arguments=["0.196", "0", "-1.076", "0", "-0.698", "0", "aft_mapped", "base_link"],
                    output='screen',
                ),
            ],
        ),
    ]

def generate_launch_description():
    auto_bringup_dir = FindPackageShare('auto_bringup')

    declared_arguments = [
        DeclareLaunchArgument(
            name='fastlivo2',
            default_value='True',
            description='Use FAST-LIVO2?',
        ),
        DeclareLaunchArgument(
            name='fastlivo2_params',
            default_value=PathJoinSubstitution([auto_bringup_dir,'params','fast_livo2','fastlivo2.yaml']),
            description='',
        ),
        DeclareLaunchArgument(
            name='img_en',
            default_value='True',
            description='Enable coloured mapping?',
        ),
        DeclareLaunchArgument(
            name='img_topic',
            default_value='/d415/color/image_raw',
            description='',
        ),
        DeclareLaunchArgument(
            name='lidar_config',
            default_value=PathJoinSubstitution([auto_bringup_dir,'params','lidar_config.json']),
            description='',
        ),
        DeclareLaunchArgument(
            name='lidar_params',
            default_value=PathJoinSubstitution([auto_bringup_dir,'params','lidar.yaml']),
            description='',
        ),
        DeclareLaunchArgument(
            name='obstacles_detection',
            default_value='True',
            description='Run RTAB-Map node for obstacle detection?',
        ),
        DeclareLaunchArgument(
            name='output_dir',
            default_value='fastlivo2',
            description='The folder to save FAST-LIVO2 outputs to, relative to ~/.ros.',
        ),
        DeclareLaunchArgument(
            name='save_dir',
            default_value='~',
            description='The path to save the zipped FAST-LIVO2 PCD files.',
        ),
        DeclareLaunchArgument(
            name='sim',
            default_value='False',
            description='Use /clock instead of system clock?',
        ),
        DeclareLaunchArgument(
            name='tfs',
            default_value='True',
            description='Publish Nav2-required transforms? (map -> odom -> base_link)',
        ),
        DeclareLaunchArgument(
            name='uncompress_img',
            default_value='False',
            description='Uncompress compressed image stream? (for playing back from rosbag)',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
