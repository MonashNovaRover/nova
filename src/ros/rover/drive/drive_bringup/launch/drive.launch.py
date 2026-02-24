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
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, IfElseSubstitution
from launch.conditions import IfCondition, UnlessCondition
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction, GroupAction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

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
    urdf = LaunchConfiguration('urdf')
    active_controller = LaunchConfiguration('active_controller')
    
    nova_bringup_dir = FindPackageShare('nova_bringup')
    auto_bringup_dir = FindPackageShare('auto_bringup')
    params = IfElseSubstitution(auto, auto_params, nova_params)

    def spawner_args(controller: str) -> list[str]:
        arguments = [controller]
        if controller != active_controller.perform(context):
            arguments.append('--inactive')
        return arguments

    return [
        Node(
            package='controller_manager',
            executable='spawner',
            arguments=[*spawner_args('pivot_drive_controller'), '--switch-timeout', '10'],
            ros_arguments=['--log-level', log_level],
        ),
        Node(
            package='controller_manager',
            executable='spawner',
            arguments=[*spawner_args('ackermann_steering_controller'),
                       '--controller-ros-args', '-r /ackermann_steering_controller/reference:=/cmd_vel'],
            ros_arguments=['--log-level', log_level],
        ),
        Node(
            package='controller_manager',
            executable='spawner',
            arguments=spawner_args('strafe_drive_controller'),
            ros_arguments=['--log-level', log_level],
        ),
        Node(
            package='controller_manager',
            executable='spawner',
            arguments=spawner_args('diff_drive_controller'),
            ros_arguments=['--log-level', log_level],
        ),
        GroupAction(
            condition=UnlessCondition(gazebo),
            actions=[
                Node(
                    package='controller_manager',
                    executable='spawner',
                    arguments=['joint_state_broadcaster',
                               '--controller-ros-args', '-r __ns:=/drive'],
                    ros_arguments=['--log-level', log_level],
                ),
                Node(
                    package='controller_manager',
                    executable='ros2_control_node',
                    parameters=[params],
                    remappings=[
                        ('/controller_manager/robot_description', '/robot_description'),
                    ],
                    ros_arguments=['--log-level', log_level],
                ),
                GroupAction(
                    condition=IfCondition(urdf),
                    actions=[
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
                    ],
                ),
                Node(
                    package='blcmd_utils', 
                    executable='status_monitor', 
                    output='screen', 
                    emulate_tty=True,
                    ros_arguments=['--log-level', log_level],
                ),
                IncludeLaunchDescription(
                    launch_description_source=PythonLaunchDescriptionSource(
                        PathJoinSubstitution([nova_bringup_dir, "launch", "can.launch.py"])
                    ),
                    launch_arguments={
                        "bus" : "can0",
                        "bitrate" : "250000",
                        "log_name" : "drive",
                    }.items()
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
            default_value='info',
            description='sets log level of all nodes started by this launch file'
                        '(set log level of individual node with "log_level:=<node name>:=<log level>")',
        ),
        DeclareLaunchArgument(
            name='model', 
            default_value=PathJoinSubstitution([rover_description_dir, 'banksia', 'urdf', 'rover.urdf.xacro']),
            description='Absolute path to robot urdf file',
        ),
        DeclareLaunchArgument(
            name='urdf',
            default_value='True',
            description='Launch URDF?',
        ),
        DeclareLaunchArgument(
            name='active_controller',
            default_value='pivot_drive_controller',
            choices=['pivot_drive_controller', 'ackermann_steering_controller',
                     'strafe_drive_controller', 'diff_drive_controller'],
            description='Name of controller that is initially active',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
