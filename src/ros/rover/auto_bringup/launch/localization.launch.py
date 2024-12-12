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
from launch.actions import IncludeLaunchDescription, DeclareLaunchArgument
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

# Generate the launch file with all inputs
# TODO: Add IMU, GPS, and any other localisation nodes


def launch_setup(context, *args, **kwargs):
    auto_bringup_dir = FindPackageShare('auto_bringup')

    # General Configuartions
    use_sim_time = LaunchConfiguration('use_sim_time')

    # Simulation Configurations
    use_real_odometry = LaunchConfiguration('use_real_odometry').perform(context).lower() == 'true'

    use_slam = LaunchConfiguration('use_slam')
    use_vo = LaunchConfiguration('use_vo')
    gps = LaunchConfiguration('gps')

    # Params File Configurations
    ekf_params = LaunchConfiguration('ekf_params_file')
    ukf_params = LaunchConfiguration('ukf_params_file')

    if LaunchConfiguration('use_ukf').perform(context).lower() == 'true':
        filter_type = 'ukf'
        params_file = ukf_params.perform(context)
    elif LaunchConfiguration('use_ukf').perform(context).lower() == 'false':
        filter_type = 'ekf'
        params_file = ekf_params.perform(context)
    else:
        raise ValueError('use_ukf must be either True or False')

    real_odom_params = {
        'odom0': '/odom/gazebo',
        'odom0_relative': False,
        'odom0_config': [True, True, True,
                         True, True, True,
                         True, False, False,
                         False, False, False,
                         False, False, False],
        'odom0_relative': True,
    }

    return [
        Node(
            condition=UnlessCondition(gps),
            package='robot_localization',
            executable=filter_type + '_node',
            name=filter_type + '_filter_node',
            output='screen',
            parameters=[(params_file),  {'use_sim_time': use_sim_time}, real_odom_params if use_real_odometry else {}],
        ),
        Node(
            condition=IfCondition(gps),
            package='robot_localization',
            executable='ekf_node',
            name='ekf_filter_node_odom',
            output='screen',
            parameters=[(LaunchConfiguration('rl_params_file').perform(context)),  {'use_sim_time': use_sim_time}],
            remappings=[('odometry/filtered', 'odometry/local')]
        ),
        Node(
            condition=IfCondition(gps),
            package='robot_localization',
            executable='ekf_node',
            name='ekf_filter_node_map',
            output='screen',
            parameters=[(LaunchConfiguration('rl_params_file').perform(context)), {'use_sim_time': use_sim_time}],
            remappings=[('odometry/filtered', 'odometry/global')]
        ),
        Node(
            condition=IfCondition(gps),
            package='robot_localization',
            executable='navsat_transform_node',
            name='navsat_transform',
            output='screen',
            parameters=[(LaunchConfiguration('rl_params_file').perform(context)), {'use_sim_time': use_sim_time}],
            remappings=[
                ('odometry/filtered', 'odometry/global'),
                ('gps/fix', 'fix'),
                ('imu', 'oak/imu/transformed')],
        ),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'rtabmap.launch.py'])),
            condition=IfCondition(use_slam),
            launch_arguments={'use_sim_time': use_sim_time}.items(),
        ),
        Node(
            package='tf2_ros',
            executable='static_transform_publisher',
            condition=UnlessCondition(OrSubstitution(use_slam, gps)),
            name='static_transform_publisher',
            output='screen',
            arguments=['0', '0', '0', '0', '0', '0', 'map', 'odom'],
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
            name='use_ukf',
            default_value='False',
            description='Use UKF (True) or EKF (False)',
        ),
        DeclareLaunchArgument(
            name='ekf_params_file',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'ekf.yaml']),
            description='Params file for ekf filter node',
        ),
        DeclareLaunchArgument(
            name='ukf_params_file',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'ukf.yaml']),
            description='Params file for ukf filter node',
        ),
        DeclareLaunchArgument(
            name='use_slam',
            default_value='False',
            description='use slam for map->odom transform',

        ),
        DeclareLaunchArgument(
            name='rl_params_file',
            default_value=PathJoinSubstitution([auto_bringup_dir,'params','rl.yaml']),
        ),
        DeclareLaunchArgument(
            name='gps',
            default_value='True',
            description='Fuse GPS?',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
