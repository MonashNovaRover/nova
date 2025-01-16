'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the rover to start all
   rover control scripts.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - control/drive/drive_inputs      [drive_cmd]
  - control/drive/driver            [driver]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	core
CREATION:	29/12/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from launch import LaunchDescription
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch.conditions import UnlessCondition
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction, GroupAction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def launch_setup(context, *args, **kwargs):
    nova_bringup_dir = FindPackageShare('nova_bringup')
    
    controllers = LaunchConfiguration('controllers')
    gazebo = LaunchConfiguration('gazebo')
    model = LaunchConfiguration('model')
    
    teleop_drive_joy_dir = FindPackageShare('teleop_drive_joy')
    auto_bringup_dir = FindPackageShare('auto_bringup')

    return [
        Node(
            package='controller_manager',
            executable='spawner',
            arguments=['pivot_drive_controller', '--switch-timeout', '10'] #, '--inactive']
        ),
        GroupAction(
            condition=UnlessCondition(gazebo),
            actions=[
                Node(
                    package='controller_manager',
                    executable='spawner',
                    arguments=['joint_state_broadcaster']
                ),
                Node(
                    package='controller_manager',
                    executable='ros2_control_node',
                    parameters=[controllers],
                    remappings=[('/controller_manager/robot_description', '/robot_description')],
                ),
                IncludeLaunchDescription(
                    PythonLaunchDescriptionSource(PathJoinSubstitution([nova_bringup_dir, 'launch', 'urdf.launch.py'])),
                    launch_arguments={'model': model, 'gazebo': gazebo}.items(),
                ),
                IncludeLaunchDescription(
                    launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([teleop_drive_joy_dir, 'launch', 'teleop.launch.py'])),
                    launch_arguments = {
                        'joystick': "ps5"
                    }.items()
                ),
                IncludeLaunchDescription(
                    launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'gazebo.launch.py'])),
                )],
        ),
    ]


def generate_launch_description():
    nova_bringup_dir = FindPackageShare('nova_bringup')
    rover_description_dir = FindPackageShare('rover_description')
    nova_gazebo_dir = FindPackageShare('nova_gazebo')
    auto_bringup_dir = FindPackageShare('auto_bringup')

    declared_arguments = [
        DeclareLaunchArgument(
            'joystick',
            default_value = 'ps5',
            description = 'Determines the type of controller used (ps5, xbox, nintendo)'
        ),

        # Gazebo
        DeclareLaunchArgument(
            name='config_file',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'gz_bridge.yaml']),
            description='Absolute path to ros_gz_bridge params file',
        ),
        DeclareLaunchArgument(
            name='launch_robot_description',
            default_value='True',
            description='Should gazebo launch its own robot description, or is one already running?',
        ),
        DeclareLaunchArgument(
            name='model',
            default_value=PathJoinSubstitution([rover_description_dir, 'base', 'urdf', 'rover.urdf.xacro']),
            description='Absolute path to robot urdf file',
        ),
        DeclareLaunchArgument(
            name='namespace',
            default_value='',
            description='Top-level namespace',
        ),
        DeclareLaunchArgument(
            name='robot_name',
            default_value='Rover7',
            description='name of the robot',
        ),
        DeclareLaunchArgument(
            name='world',
            default_value=PathJoinSubstitution([nova_gazebo_dir, 'worlds', 'auto.sdf']),
            description='Full path to world model file to load',
        ),
        DeclareLaunchArgument(name='x', default_value='11.2123871', description='x_pose'),
        DeclareLaunchArgument(name='y', default_value='-10.1349831', description='y_pose'),
        DeclareLaunchArgument(name='z', default_value='0.5', description='z_pose'),
        DeclareLaunchArgument(name='R', default_value='0.0', description='roll'),
        DeclareLaunchArgument(name='P', default_value='0.0', description='pitch'),
        DeclareLaunchArgument(name='Y', default_value='2.5740044', description='yaw'),
    ]


    declared_arguments = [      
        DeclareLaunchArgument(
            name='controllers',
            default_value=PathJoinSubstitution([nova_bringup_dir, 'params', 'controllers.yaml']),
            description='Absolute path to controller params file',
        ),
        DeclareLaunchArgument(
            name='gazebo',
            default_value='False',
            description='Use simulation (Gazebo) clock if True',
        ),
        DeclareLaunchArgument(
            name='model', 
            default_value=PathJoinSubstitution([rover_description_dir, 'base', 'urdf', 'auger_ros2_control.xacro']),
            description='Absolute path to robot urdf file',
        ),
        DeclareLaunchArgument(
            'joystick',
            default_value = 'ps5',
            description = 'Determines the type of controller used (ps5, xbox, nintendo)'
        ),

        # Gazebo
        DeclareLaunchArgument(
            name='config_file',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'gz_bridge.yaml']),
            description='Absolute path to ros_gz_bridge params file',
        ),
        DeclareLaunchArgument(
            name='launch_robot_description',
            default_value='True',
            description='Should gazebo launch its own robot description, or is one already running?',
        ),
        #DeclareLaunchArgument(
        #    name='model',
        #    default_value=PathJoinSubstitution([rover_description_dir, 'base', 'urdf', 'rover.urdf.xacro']),
        #    description='Absolute path to robot urdf file',
        #),
        DeclareLaunchArgument(
            name='namespace',
            default_value='',
            description='Top-level namespace',
        ),
        DeclareLaunchArgument(
            name='robot_name',
            default_value='Rover7',
            description='name of the robot',
        ),
        DeclareLaunchArgument(
            name='world',
            default_value=PathJoinSubstitution([nova_gazebo_dir, 'worlds', 'auto.sdf']),
            description='Full path to world model file to load',
        ),
        DeclareLaunchArgument(name='x', default_value='11.2123871', description='x_pose'),
        DeclareLaunchArgument(name='y', default_value='-10.1349831', description='y_pose'),
        DeclareLaunchArgument(name='z', default_value='0.5', description='z_pose'),
        DeclareLaunchArgument(name='R', default_value='0.0', description='roll'),
        DeclareLaunchArgument(name='P', default_value='0.0', description='pitch'),
        DeclareLaunchArgument(name='Y', default_value='2.5740044', description='yaw'),

    ]

    return LaunchDescription(  
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
