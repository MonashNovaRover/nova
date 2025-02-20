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
from launch.actions import DeclareLaunchArgument, OpaqueFunction, GroupAction
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch.conditions import IfCondition, UnlessCondition
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    ekf_params = LaunchConfiguration('ekf_params').perform(context)
    gps = LaunchConfiguration('gps')
    rl_params = LaunchConfiguration('rl_params').perform(context)
    ukf_params = LaunchConfiguration('ukf_params').perform(context)
    use_real_odometry = (LaunchConfiguration('use_real_odometry').perform(context).lower() == 'true')
    use_sim_time = (LaunchConfiguration('use_sim_time').perform(context).lower() == 'true')
    use_ukf = (LaunchConfiguration('use_ukf').perform(context).lower() == 'true')

    if use_ukf:
        filter_type = 'ukf'
        gps_params = ukf_params
    elif not use_ukf:
        filter_type = 'ekf'
        gps_params = ekf_params
    else:
        raise ValueError('use_ukf must be either True or False')

    real_odom_params = {
        'odom0': '/odom/gazebo',
        'odom0_relative': True,
        'odom0_config': [True, True, True,
                         True, True, True,
                         False, False, False,
                         False, False, False,
                         False, False, False],
    }

    return [
        Node(
            condition=UnlessCondition(gps),
            package='robot_localization',
            executable=f'{filter_type}_node',
            name=f'{filter_type}_filter_node',
            output='screen',
            parameters=[gps_params,  {'use_sim_time': use_sim_time}, real_odom_params if use_real_odometry else {}],
        ),
        GroupAction(
            condition=IfCondition(gps),
            actions=[
                Node(
                    package='robot_localization',
                    executable='ekf_node',
                    name='ekf_filter_node_odom',
                    output='screen',
                    parameters=[rl_params,  {'use_sim_time': use_sim_time}],
                    remappings=[('odometry/filtered', 'odometry/local')],
                ),
                Node(
                    package='robot_localization',
                    executable='ekf_node',
                    name='ekf_filter_node_map',
                    output='screen',
                    parameters=[rl_params, {'use_sim_time': use_sim_time}],
                    remappings=[('odometry/filtered', 'odometry/global')],
                ),
                Node(
                    package='robot_localization',
                    executable='navsat_transform_node',
                    name='navsat_transform',
                    output='screen',
                    parameters=[rl_params, {'use_sim_time': use_sim_time}],
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
            name='ekf_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'ekf.yaml']),
            description='Params file for ekf filter node',
        ),
        DeclareLaunchArgument(
            name='gps',
            default_value='False',
            description='Fuse GPS?',
        ),
        DeclareLaunchArgument(
            name='rl_params',
            default_value=PathJoinSubstitution([auto_bringup_dir,'params','rl.yaml']),
            description='',
        ),
        DeclareLaunchArgument(
            name='ukf_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'ukf.yaml']),
            description='Params file for ukf filter node',
        ),
        DeclareLaunchArgument(
            name='use_real_odometry',
            default_value='False',
            description='Use the ground truth odometry from gazebo',
        ),
        DeclareLaunchArgument(
            name='use_sim_time',
            default_value='False',
            description='Use simulation clock if True',
        ),
        DeclareLaunchArgument(
            name='use_ukf',
            default_value='False',
            description='Use UKF (True) or EKF (False)',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
