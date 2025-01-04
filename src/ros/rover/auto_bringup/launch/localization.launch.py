'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the rover to publish the urdf
    static transforms and associated joint states
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - robot_state_publisher
  - rover_state_publisher
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	core
CREATION:	27/04/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.substitutions import LaunchConfiguration, OrSubstitution, PathJoinSubstitution
from launch.conditions import IfCondition, UnlessCondition
from launch.actions import IncludeLaunchDescription, DeclareLaunchArgument, GroupAction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

# Generate the launch file with all inputs
# TODO: Add IMU, GPS, and any other localisation nodes

def launch_setup(context, *args, **kwargs):
    auto_bringup_dir = FindPackageShare('auto_bringup')

    use_sim_time = LaunchConfiguration('use_sim_time')
    use_real_odometry = LaunchConfiguration('use_real_odometry').perform(context).lower() == 'true'
    slam = LaunchConfiguration('slam')
    use_vo = LaunchConfiguration('use_vo')
    gps = LaunchConfiguration('gps')
    params = LaunchConfiguration('params').perform(context)

    real_odom_params = {
        'odom0': '/odom',
        'odom0_relative': False,
        'odom0_config': [True, True, True,
                         True, True, True,
                         True, False, False,
                         False, False, False,
                         False, False, False],
        'odom0_relative': True,
    }

    return [
        IncludeLaunchDescription(
            condition=IfCondition(slam),
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'rtabmap.launch.py'])),
            launch_arguments={'use_sim_time': use_sim_time}.items(),
        ),
        GroupAction(
            condition=UnlessCondition(gps),
            actions=[
                Node(
                    package='robot_localization',
                    executable='ekf_node',
                    name='ekf_filter_node_odom',
                    output='screen',
                    parameters=[(params),  {'use_sim_time': use_sim_time}, real_odom_params if use_real_odometry else {}],
                ),
                Node(
                    condition=UnlessCondition(slam),
                    package='tf2_ros',
                    executable='static_transform_publisher',
                    name='static_transform_publisher',
                    output='screen',
                    arguments=['0', '0', '0', '0', '0', '0', 'map', 'odom'],
                ),
            ],
        ),
        GroupAction(
            condition=IfCondition(gps),
            actions=[
                Node(
                    package='robot_localization',
                    executable='ekf_node',
                    name='ekf_filter_node_odom',
                    output='screen',
                    parameters=[(params),  {'use_sim_time': use_sim_time}],
                    remappings=[('odometry/filtered', 'odometry/local')]
                ),
                Node(
                    package='robot_localization',
                    executable='ekf_node',
                    name='ekf_filter_node_map',
                    output='screen',
                    parameters=[(params), {'use_sim_time': use_sim_time}],
                    remappings=[('odometry/filtered', 'odometry/global')]
                ),
                Node(
                    package='robot_localization',
                    executable='navsat_transform_node',
                    name='navsat_transform',
                    output='screen',
                    parameters=[(params), {'use_sim_time': use_sim_time}],
                    remappings=[
                        ('odometry/filtered', 'odometry/global'),
                        ('gps/fix', 'fix'),
                        ('imu', 'oak/imu/transformed')],
                ),
            ],
        ),
    ]

def generate_launch_description():
    auto_bringup_dir = FindPackageShare('auto_bringup')

    declared_arguments = [
        DeclareLaunchArgument(
            name='use_sim_time',
            default_value='False',
            description='Use simulation clock if True',
        ),
        DeclareLaunchArgument(
            name='use_real_odometry',
            default_value='False',
            description='Use the ground truth odometry from gazebo',
        ),
        DeclareLaunchArgument(
            name='slam',
            default_value='False',
            description='use slam for map->odom transform',
        ),
        DeclareLaunchArgument(
            name='gps',
            default_value='False',
            description='Fuse GPS?',
        ),
        DeclareLaunchArgument(
            name='params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'ekf.yaml']),
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )