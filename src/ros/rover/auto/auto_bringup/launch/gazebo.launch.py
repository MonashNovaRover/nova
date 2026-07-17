
'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This launch file is responsible for bringing up the
gazebo simulation environment.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
INCLUDED LAUNCH FILES:
- drive.launch.py
- urdf.launch.py
- gz_sim.launch.py

NODES:
  - ros_gz_sim/create
  - ros_gz_bridge/bridge_node
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	auto_bringup
CREATION:	27/04/2023
EDITED:     05/01/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, AppendEnvironmentVariable, OpaqueFunction, GroupAction
from launch.conditions import IfCondition
from launch.substitutions import PathJoinSubstitution, LaunchConfiguration, IfElseSubstitution, EnvironmentVariable
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

from os.path import expanduser

def launch_setup(context, *args, **kwargs):
    # package directories
    local = LaunchConfiguration('local')

    auto_bringup_dir = IfElseSubstitution(local,
        PathJoinSubstitution([expanduser("~") + '/nova/src/ros/rover/auto/auto_bringup']),
        FindPackageShare('auto_bringup')
    )
    nova_gazebo_dir = IfElseSubstitution(local,
        PathJoinSubstitution([expanduser("~") + '/nova/src/ros/rover/simulations/nova_gazebo']),
        FindPackageShare('nova_gazebo')
    )
    drive_bringup_dir = IfElseSubstitution(local,
        PathJoinSubstitution([expanduser("~") + '/nova/src/ros/rover/drive/drive_bringup']),
        FindPackageShare('drive_bringup')
    )
    ros_gz_sim_dir = FindPackageShare('ros_gz_sim')

    comp = LaunchConfiguration('comp').perform(context).lower()

    # comp agnostic arguments
    gz_params = LaunchConfiguration('gz_params')
    gz_qos_params = LaunchConfiguration('gz_qos_params')
    log_level = LaunchConfiguration('log_level')
    urdf_path = LaunchConfiguration('urdf_path')
    namespace = LaunchConfiguration('namespace')
    pose = {'x': LaunchConfiguration('x').perform(context),
            'y': LaunchConfiguration('y').perform(context),
            'z': LaunchConfiguration('z').perform(context),
            'R': LaunchConfiguration('R').perform(context),
            'P': LaunchConfiguration('P').perform(context),
            'Y': LaunchConfiguration('Y').perform(context)}
    robot_name = LaunchConfiguration('robot_name')
    world = LaunchConfiguration('world')
    rviz = LaunchConfiguration('rviz')
    rviz_params = LaunchConfiguration('rviz_params')

    # comp defaults
    if comp == 'arch':
        world = PathJoinSubstitution([nova_gazebo_dir, 'worlds', 'auto_cubes.sdf'])
    elif comp == 'urc':
        world = PathJoinSubstitution([nova_gazebo_dir, 'worlds', 'urc_obstacles.sdf'])
    else:
        raise ValueError('"comp" arg must be either "arch" or "urc"')

    # comp defaults overrides
    if LaunchConfiguration('world').perform(context) != '':
        world = LaunchConfiguration('world')

    return [
        AppendEnvironmentVariable(
            name='GZ_SIM_RESOURCE_PATH',
            value=PathJoinSubstitution([nova_gazebo_dir, 'models'])
        ),
        AppendEnvironmentVariable(
            name='GZ_SIM_RESOURCE_PATH',
            value=PathJoinSubstitution([nova_gazebo_dir, 'worlds'])
        ),
        IncludeLaunchDescription(
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([drive_bringup_dir, 'launch', 'drive.launch.py'])),
            launch_arguments={'local': local, 'urdf': 'False', 'auto': 'True', 'sim': 'True'}.items(),
        ),
        IncludeLaunchDescription(
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'urdf.launch.py'])),
            launch_arguments={'local': local, 'urdf_path': urdf_path, 'sim': 'true', 'robot_name': robot_name, 'rviz': rviz, 'rviz_params': rviz_params}.items(),
        ),
        IncludeLaunchDescription(
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([ros_gz_sim_dir, 'launch', 'gz_sim.launch.py'])),
            launch_arguments={'gz_args': ['-r -v1 ', world], 'on_exit_shutdown': 'True'}.items(),
        ),
        Node(
            package='ros_gz_sim',
            executable='create',
            output='screen',
            arguments=[
                '-topic', 'robot_description',
                '-name', robot_name,
                '-robot_namespace', namespace,
                '-x', pose['x'], '-y', pose['y'], '-z', pose['z'],
                '-R', pose['R'], '-P', pose['P'], '-Y', pose['Y']],
            ros_arguments=['--log-level', log_level],
        ),
        Node(
            package='ros_gz_bridge',
            executable='bridge_node',
            name='ros_gz_bridge',
            namespace=namespace,
            output='screen',
            respawn=False,
            respawn_delay=2.0,
            parameters=[{'config_file': gz_params}, gz_qos_params],
            ros_arguments=['--log-level', log_level],
        ),
        GroupAction(
            condition=IfCondition(str(comp == 'urc')),
            actions=[
                Node(
                    package='nova_utils', 
                    executable='gz_gps_fixer.py', 
                    output='screen', 
                    emulate_tty=True,
                    ros_arguments=['--log-level', log_level],
                ),
                Node(
                    package='nova_utils',
                    executable='gz_heading_imu_fixer.py',
                    output='screen',
                    emulate_tty=True,
                    ros_arguments=['--log-level', log_level],
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
    drive_bringup_dir = IfElseSubstitution(local,
        PathJoinSubstitution([expanduser("~") + '/nova/src/ros/rover/drive/drive_bringup']),
        FindPackageShare('drive_bringup')
    )
    rover_description_dir = IfElseSubstitution(local,
        PathJoinSubstitution([expanduser("~") + '/nova/src/ros/rover/rover_description']),
        FindPackageShare('rover_description')
    )

    declared_arguments = [
        DeclareLaunchArgument(
            name='local',
            default_value='False',
            description='Whether to use local directories instead of the nix store.',
        ),
        DeclareLaunchArgument(
            name='comp',
            default_value=EnvironmentVariable('COMP', default_value='URC'),
            description='ARCh or URC',
        ),
        # comp agnostic arguments
        DeclareLaunchArgument(
            name='gz_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'gz_bridge.yaml']),
            description='Absolute path to ros_gz_bridge params file',
        ),
        DeclareLaunchArgument(
            name='gz_qos_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'gz_bridge_qos.yaml']),
            description='Absolute path to ros_gz_bridge params file',
        ),
        DeclareLaunchArgument(
            name='log_level',
            default_value='info',
            description='What level of logging output should be displayed',
        ),
        DeclareLaunchArgument(
            name='urdf_path',
            default_value=PathJoinSubstitution([rover_description_dir, 'banksia', 'urdf', 'rover.urdf.xacro']),
            description='Absolute path to robot urdf file',
        ),
        DeclareLaunchArgument(
            name='namespace',
            default_value='',
            description='Top-level namespace',
        ),
        DeclareLaunchArgument(
            name='robot_name',
            default_value='Banksia',
            description='name of the robot',
        ),
        DeclareLaunchArgument(
            name='rviz',
            default_value='False',
            description='Whether to launch RViz',
        ),
        DeclareLaunchArgument( # Do not include 'rviz' argument in nested launch files https://github.com/ros2/launch/issues/313
            name='rviz_params',
            default_value='everything',
            description='Name of the rviz config file to use, without the .rviz extension. Must be located in src/ros/rover/auto/auto_bringup/rviz',
        ),
        DeclareLaunchArgument(name='x', default_value='-3.0', description='x_pose'),
        DeclareLaunchArgument(name='y', default_value='-2.0', description='y_pose'),
        DeclareLaunchArgument(name='z', default_value='0.5', description='z_pose'),
        DeclareLaunchArgument(name='R', default_value='0.0', description='roll'),
        DeclareLaunchArgument(name='P', default_value='0.0', description='pitch'),
        DeclareLaunchArgument(name='Y', default_value='0.0', description='yaw'),
        # arguments with comp defaults
        DeclareLaunchArgument(
            name='world',
            default_value='',
            description='Full path to world model file to load',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )