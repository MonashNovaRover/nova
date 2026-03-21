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
from os.path import expanduser

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction, GroupAction, RegisterEventHandler
from launch.event_handlers import OnProcessExit
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, IfElseSubstitution, AndSubstitution, NotSubstitution
from launch.conditions import IfCondition, UnlessCondition
from launch.logging import get_logger
from launch_ros.actions import Node, SetParameter, LoadComposableNodes
from launch_ros.descriptions import ComposableNode
from launch_ros.substitutions import FindPackageShare
from nav2_common.launch import RewrittenYaml


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
    # package directories
    local = LaunchConfiguration('local')

    auto_bringup_dir = IfElseSubstitution(local,
        PathJoinSubstitution([expanduser("~") + '/nova/src/ros/rover/auto/auto_bringup']),
        FindPackageShare('auto_bringup')
    )

    new_container = LaunchConfiguration('new_container')
    container_name = LaunchConfiguration('container_name')
    driver = LaunchConfiguration('driver')
    lidar_config = LaunchConfiguration('lidar_config').perform(context)
    lidar_params = LaunchConfiguration('lidar_params')
    mask = LaunchConfiguration('mask')
    ground_seg = LaunchConfiguration('ground_seg')
    ground_seg_params = LaunchConfiguration('ground_seg_params')
    tfs = LaunchConfiguration('tfs')
    fastlivo2 = LaunchConfiguration('fastlivo2')
    fastlivo2_params = LaunchConfiguration('fastlivo2_params')
    img_en = int(LaunchConfiguration('img_en').perform(context).lower() == 'true')
    sim = LaunchConfiguration('sim')
    uncompress_img = LaunchConfiguration('uncompress_img')
    shortened_auto_mount = LaunchConfiguration('shortened_auto_mount')

    img_topic = '/d415/color/image_raw'
    lid_topic = '/livox/lidar_masked'
    imu_topic = '/livox/imu'
    intrinsics_params = PathJoinSubstitution([auto_bringup_dir,'params','fast_livo2','d415_intrinsics.yaml'])
    extrinsics_params = PathJoinSubstitution([auto_bringup_dir,'params','fast_livo2','d415_extrinsics.yaml'])
    output_dir = 'fastlivo2' # relative to ~/.ros
    logger = get_logger("lidar_launch")

    fastlivo2_rewritten_params = RewrittenYaml(
        source_file=fastlivo2_params,
        param_rewrites={
            'img_en': str(img_en),
            'img_topic': img_topic,
            'lid_topic': lid_topic,
            'imu_topic': imu_topic,
            'scan_line': '4' if sim.perform(context).lower() == 'false' else '40',
        },
        convert_types=True,
    )

    fastlivo2_node = Node(
        package='fast_livo',
        executable='fastlivo_mapping',
        name='fastlivo2',
        output='screen',
        # gives 66.67% of CPU under 100% load assuming all other processes have default weight of 100
        prefix='systemd-run --scope --user -p CPUWeight=200 --unit=fastlivo2',
        parameters=[fastlivo2_rewritten_params, extrinsics_params,
                    {'save_folder': output_dir}],
        remappings=[('/aft_mapped_to_init', '/odometry/filtered')],
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
        SetParameter(name='use_sim_time', value=sim),
        Node(
            condition=IfCondition(AndSubstitution(driver, NotSubstitution(sim))),
            package='livox_ros_driver2',
            executable='livox_ros_driver2_node',
            name='livox_lidar_publisher',
            output='screen',
            parameters=[lidar_params, {'user_config_path': lidar_config}],
        ),
        Node(
            condition=IfCondition(new_container),
            name=container_name,
            package='rclcpp_components',
            executable='component_container_isolated',
            output='screen',
        ),
        LoadComposableNodes(
            target_container=container_name,
            composable_node_descriptions=[
                ComposableNode(
                    condition=IfCondition(mask),
                    package='pcl_ros',
                    plugin='pcl_ros::CropBox',
                    name='crop_box_filter',
                    parameters=[{'min_x': -0.64, 'max_x': 0.64,
                                 'min_y': -0.57, 'max_y': 0.57,
                                 'min_z': 0.0, 'max_z': 4.0,
                                 'negative': True,
                                 'input_frame': 'base_link'}],
                    remappings=[('input', '/livox/lidar'),
                                ('output', '/livox/lidar_masked')],
                    extra_arguments=[{'use_intra_process_comms': True}],
                ),
                ComposableNode(
                    condition=IfCondition(ground_seg),
                    package='ground_segmentation_ros2',
                    plugin='GroundSegmentatioNode',
                    name='ground_segmentation',
                    parameters=[ground_seg_params],
                    remappings=[
                        ('/ground_segmentation/input_pointcloud', '/livox/lidar_masked'),
                        ('/ground_segmentation/input_imu', '/livox/imu'),
                    ],
                    extra_arguments=[{'use_intra_process_comms': True}],
                ),
            ],
        ),
        Node(
            condition=IfCondition(sim),
            package='nova_utils',
            executable='livox_field_republisher.py',
            name='livox_field_republisher',
            output='screen',
            parameters=[
                {'input_topic': '/livox/lidar_sim'},
                {'output_topic': '/livox/lidar'},
                {'default_tag': 16},
            ],
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
                    parameters=[intrinsics_params],
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
                                    'save_dir': '~',
                                    'logger': logger}
                        ),
                    ),
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
                GroupAction(
                    condition=UnlessCondition(shortened_auto_mount),
                    actions=[
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
                GroupAction(
                    condition=IfCondition(shortened_auto_mount),
                    actions=[
                        Node(
                            package='tf2_ros',
                            executable='static_transform_publisher',
                            name='odom_to_camera_init_publisher',
                            # tf2_echo base_link to livox_frame
                            arguments=["0.330", "0", "0.950", "0", "0", "0", "odom", "camera_init"],
                            output='screen',
                        ),
                        Node(
                            package='tf2_ros',
                            executable='static_transform_publisher',
                            name='aft_mapped_to_base_link_publisher',
                            # tf2_echo livox_frame to base_link
                            arguments=["0.358", "0", "-0.940", "0", "-0.698", "0", "aft_mapped", "base_link"],
                            output='screen',
                        ),
                    ],
                ),
            ],
        ),
    ]

def generate_launch_description():
    local = LaunchConfiguration('local')
    
    auto_bringup_dir = IfElseSubstitution(local,
        PathJoinSubstitution([expanduser("~") + '/nova/src/ros/rover/auto/auto_bringup']),
        FindPackageShare('auto_bringup')
    )

    declared_arguments = [
        DeclareLaunchArgument(
            name='local',
            default_value='False',
            description='Whether to use local directories instead of the nix store.',
        ),
        DeclareLaunchArgument(
            name='new_container',
            default_value='True',
            description='Whether to start a new component container.',
        ),
        DeclareLaunchArgument(
            name='container_name',
            default_value='auto_container',
            description='Name of the component container.',
        ),
        DeclareLaunchArgument(
            name='driver',
            default_value='True',
            description='Launch livox_ros_driver2?',
        ),
        DeclareLaunchArgument(
            name='mask',
            default_value='True',
            description='Remove points that intersect with the rover?',
        ),
        DeclareLaunchArgument(
            name='ground_seg',
            default_value='True',
            description='Run ground segmentation?',
        ),
        DeclareLaunchArgument(
            name='ground_seg_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'ground_segmentation.yaml']),
            description='Full path to the parameters file to use for ground segmentation',
        ),
        DeclareLaunchArgument(
            name='tfs',
            default_value='True',
            description='Publish Nav2-required transforms? (map -> odom -> base_link)',
        ),
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
            name='uncompress_img',
            default_value='False',
            description='Uncompress compressed image stream? (for playing back from rosbag)',
        ),
        DeclareLaunchArgument(
            name='shortened_auto_mount',
            default_value='True',
            description='Use shortened auto mount TFs?',
        ),
        DeclareLaunchArgument(
            name='sim',
            default_value='False',
            description='Use /clock instead of system clock?',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
