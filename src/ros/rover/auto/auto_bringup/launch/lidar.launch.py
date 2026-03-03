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
    Chessum, Victor Bartlinski
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from pathlib import Path
import shutil
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction, GroupAction, RegisterEventHandler
from launch.event_handlers import OnProcessExit
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch.conditions import IfCondition, UnlessCondition
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

# Similar to RTABMap, we want to delete previous maps when running FAST-LIVO2
# due to their large size and limited disk space on the device being run. 
def delete_fastlivo2_pcd(context, save_folder='fastlivo2'):
    dir_path = Path(f'~/.ros/{save_folder}/pcd').expanduser()
    if dir_path.exists() and dir_path.is_dir():
        for item in dir_path.iterdir():
            if item.is_file():
                item.unlink()
            else:
                print(f"Warning: {item} is not a file and was not deleted.")
    else:
        print(f"Warning: Directory {dir_path} does not exist.")

def zip_fastlivo2_pcd(context, save_folder='fastlivo2', zip_path='~/pcd.zip'):
    dir_path = Path(f'~/.ros/{save_folder}/pcd').expanduser()
    zip_path = Path(zip_path).expanduser()
    if dir_path.exists() and dir_path.is_dir():
        shutil.make_archive(
            base_name=zip_path,
            format='zip',
            root_dir=dir_path
        )
    else:
        print(f"Warning: Directory {dir_path} does not exist and could not be zipped.")

def launch_setup(context, *args, **kwargs):
    intrinsics_params = LaunchConfiguration('intrinsics_params')
    extrinsics_params = LaunchConfiguration('extrinsics_params')
    img_topic = LaunchConfiguration('img_topic').perform(context)
    fastcalib = LaunchConfiguration('fastcalib')
    fastcalib_params = LaunchConfiguration('fastcalib_params')
    fastlivo2 = LaunchConfiguration('fastlivo2')
    fastlivo2_params = LaunchConfiguration('fastlivo2_params')
    save_folder = LaunchConfiguration('save_folder').perform(context)
    zip_path = LaunchConfiguration('zip_path').perform(context)
    lidar_config = LaunchConfiguration('lidar_config').perform(context)
    lidar_params = LaunchConfiguration('lidar_params')
    tfs = LaunchConfiguration('tfs')
    sim = LaunchConfiguration('sim')

    fastlivo2_node = Node(
        package='fast_livo',
        executable='fastlivo_mapping',
        name='fastlivo2',
        output='screen',
        parameters=[fastlivo2_params, extrinsics_params,
                    {'use_sim_time': sim,
                     'save_folder': save_folder,
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
        # NOTE image_transport only creates subscribers if subscribers exist for its publishers. 
            package="image_transport",
            executable="republish",
            name="republish",
            output="screen",
            parameters=[{'in_transport': 'compressed', 
                         'out_transport': 'raw'}],
            remappings=[("in/compressed",  "/d415/color/image_raw/compressed"), 
                        ("out", "/d415/color/image_raw")],
        ),
        Node(
            condition=IfCondition(fastcalib),
            package='fast_calib',
            executable='fast_calib',
            name='mono_qr_pattern',
            output='screen',
            parameters=[fastcalib_params]
        ),
        GroupAction(
            condition=IfCondition(fastlivo2),
            actions=[
                OpaqueFunction(
                    function=delete_fastlivo2_pcd,
                    kwargs={'save_folder': save_folder}
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
                        on_exit=fastlivo2_node,
                    ),
                ),
                RegisterEventHandler(
                    event_handler=OnProcessExit(
                        target_action=fastlivo2_node,
                        on_exit=OpaqueFunction(
                            function=zip_fastlivo2_pcd,
                            kwargs={'save_folder': save_folder, 'zip_path': zip_path}
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
            name='intrinsics_params',
            default_value=PathJoinSubstitution([auto_bringup_dir,'params','fast_livo2','d415_intrinsics.yaml']),
            description='',
        ),
        DeclareLaunchArgument(
            name='extrinsics_params',
            default_value=PathJoinSubstitution([auto_bringup_dir,'params','fast_livo2','d415_extrinsics.yaml']),
            description='',
        ),
        DeclareLaunchArgument(
            name='img_topic',
            default_value='/d415/color/image_raw',
            description='',
        ),
        DeclareLaunchArgument(
            name='fastcalib',
            default_value='False',
            description='Use FAST-Calib?',
        ),
        DeclareLaunchArgument(
            name='fastcalib_params',
            default_value=PathJoinSubstitution([auto_bringup_dir,'params','fast_livo2','fastcalib.yaml']),
            description='',
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
            name='save_folder',
            default_value='fastlivo2',
            description='The folder to save FAST-LIVO2 outputs to, relative to ~/.ros.',
        ),
        DeclareLaunchArgument(
            name='zip_path',
            default_value='~/pcd',
            description='The path to save the zipped FAST-LIVO2 PCD files.',
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
            name='tfs',
            default_value='True',
            description='Publish Nav2-required transforms? (map -> odom -> base_link)',
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
