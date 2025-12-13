'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the rover to start all
   rover control scripts.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - controller_manager/spawner
  - controller_manager/ros2_control_node
  - blcmd_utils/status_monitor
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CREATION:   15/12/2021
EDITED:     23/09/2025
EDITED BY:  Max Tory, Taaj Street, 
            Victor Bartlinski, Jared Landau,
            Bailey Chessum, Terry Tian
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from launch import LaunchDescription
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, IfElseSubstitution, PythonExpression
from launch.conditions import IfCondition, UnlessCondition
from launch.actions import LogInfo, DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction, GroupAction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
import subprocess

def setup_can(context, *args, **kwargs):
    def can_is_up():
        try:
            result = subprocess.run(["ip", "link", "show", "can0"],
                capture_output=True, text=True, check=True)
            return "UP" in result.stdout
        except subprocess.CalledProcessError:
            return False

    gazebo = LaunchConfiguration('gazebo').perform(context)  # get argument value as string
    if gazebo.lower() == 'false':
        if not can_is_up():
            try:
                subprocess.run(["can", "start", "can0"], check=True)
                return [LogInfo(msg="can0 started successfully")]
            except subprocess.CalledProcessError as e:
                return [LogInfo(msg=f"Failed to start can0: {e}")]
        else:
            return [LogInfo(msg="can0 is already running")]
    
    return []

def launch_setup(context, *args, **kwargs):
    auto = LaunchConfiguration('auto')
    nova_params = LaunchConfiguration('nova_params')
    auto_params = LaunchConfiguration('auto_params')
    
    # nova-specific arguments
    arm = LaunchConfiguration('arm')
    rviz = LaunchConfiguration('rviz')
    
    # auto-specific arguments
    angle = LaunchConfiguration('angle')
    
    gazebo = LaunchConfiguration('gazebo')
    log_level = LaunchConfiguration('log_level')
    model = LaunchConfiguration('model')
    
    nova_bringup_dir = FindPackageShare('nova_bringup')
    auto_bringup_dir = FindPackageShare('auto_bringup')
    params = IfElseSubstitution(auto, auto_params, nova_params)

    return [
        Node(
            package='controller_manager',
            executable='spawner',
            arguments=['pivot_drive_controller', '--switch-timeout', '10',
                '--ros-args', '--log-level', log_level]
        ),
        Node(
            package='controller_manager',
            executable='spawner',
            arguments=['holonomic_drive_controller', '--inactive']
        ),
        Node(
            package='controller_manager',
            executable='spawner',
            arguments=['strafe_drive_controller', '--inactive']
        ),
        Node(
            package='controller_manager',
            executable='spawner',
            arguments=['diff_drive_controller', '--inactive']
        ),
        GroupAction(
            condition=UnlessCondition(gazebo),
            actions=[
                Node(
                    package='controller_manager',
                    executable='spawner',
                    arguments=['joint_state_broadcaster'],
                ),
                Node(
                    package='controller_manager',
                    executable='ros2_control_node',
                    parameters=[params],
                    remappings=[('/controller_manager/robot_description', '/robot_description')],
                ),
                IncludeLaunchDescription(
                    condition=IfCondition(auto),
                    launch_description_source=PythonLaunchDescriptionSource(
                        PathJoinSubstitution([auto_bringup_dir, 'launch', 'urdf.launch.py'])),
                    launch_arguments={'model': model, 'angle': angle}.items(),
                ),
                IncludeLaunchDescription(
                    condition=UnlessCondition(auto),
                    launch_description_source=PythonLaunchDescriptionSource(
                        PathJoinSubstitution([nova_bringup_dir, 'launch', 'urdf.launch.py'])),
                    launch_arguments = {
                        'arm': arm,
                        'rover': 'True',
                        'rviz': rviz
                    }.items(),
                ),
                Node(
                    package='blcmd_utils', 
                    executable='status_monitor', 
                    output='screen', 
                    emulate_tty=True,
                ),
            ],
        ),
    ]


def generate_launch_description():
    drive_bringup_dir = FindPackageShare('drive_bringup')
    rover_description_dir = FindPackageShare('rover_description')

    declared_arguments = [   
        DeclareLaunchArgument(
            name='auto',
            default_value='False',
            description='Autonomous mode?',
        ),
        DeclareLaunchArgument(
            name='nova_params',
            default_value=PathJoinSubstitution([drive_bringup_dir, 'params', 'nova.yaml']),
            description='Absolute path to the nova params file',
        ),
        DeclareLaunchArgument(
            name='auto_params',
            default_value=PathJoinSubstitution([drive_bringup_dir, 'params', 'auto.yaml']),
            description='Absolute path to the auto params file',
        ),

        # These arguments are passed to the nova_bringup urdf.launch.py file
        # and are only relevant if auto is false
        DeclareLaunchArgument(
            name='arm', 
            default_value='False',
            description='Include arm URDF in robot_description?',
        ),
        DeclareLaunchArgument(
            name='rviz', 
            default_value='False',
            description='Launch rviz?',
        ),

        # This parameter is passed to the auto_bringup urdf.launch.py file
        # and is only relevant if auto is true
        DeclareLaunchArgument(
            name='angle', 
            default_value='15',
            description='Angle (in degrees) at which the camera is mounted',
        ),

        DeclareLaunchArgument(
            name='gazebo',
            default_value='False',
            description='Use simulation (Gazebo) clock if True',
        ),
        DeclareLaunchArgument(
            name='log_level',
            default_value='warn',
            description='',
        ),
        DeclareLaunchArgument(
            name='model', 
            default_value=PathJoinSubstitution([rover_description_dir, 'banksia', 'urdf', 'rover.urdf.xacro']),
            description='Absolute path to robot urdf file',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup), OpaqueFunction(function=setup_can)]
    )
