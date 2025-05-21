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
CREATION:	15/12/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from launch import LaunchDescription
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch.conditions import IfCondition, UnlessCondition
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction, GroupAction, ExecuteProcess, LogInfo
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from launch_ros.parameter_descriptions import ParameterValue


def launch_setup(context, *args, **kwargs):
    arm_bringup_dir = FindPackageShare('arm_bringup')

    controllers = LaunchConfiguration('controllers')
    gazebo = LaunchConfiguration('gazebo')
    log_level = LaunchConfiguration('log_level')
    model = LaunchConfiguration('model')
    arm = LaunchConfiguration('arm').perform(context)
    old_arm = LaunchConfiguration('old_arm').perform(context)
    use_mock_hardware = LaunchConfiguration('use_mock_hardware')

    return [
        GroupAction(
            condition=IfCondition(arm),
            actions=[
                Node(
                    package='controller_manager',
                    executable='spawner',
                    arguments=['nova_arm_velocity_controller', '--switch-timeout', '10'] #, '--inactive']
                ),
                Node(
                    package='controller_manager',
                    executable='spawner',
                    arguments=['nova_arm_position_controller', '--inactive']
                ),
                Node(
                    package='controller_manager',
                    executable='spawner',
                    arguments=['nova_twistmapper', '--inactive'],
                ),
            ]
        ),
        Node(
            package='controller_manager',
            executable='spawner',
            arguments=['pivot_drive_controller', '--switch-timeout', '10', '--ros-args', '--log-level', log_level] #, '--inactive']
        ),
        Node(
            package='controller_manager',
            executable='spawner',
            arguments=['strafe_controller', '--inactive']
        ),
        Node(
            package='controller_manager',
            executable='spawner',
            arguments=['nova_diff_drive_controller', '--inactive']
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
                    parameters=[controllers],
                    remappings=[('/controller_manager/robot_description', '/robot_description'), ('/joint_states', '/arm/joint_states')],
                ),
                IncludeLaunchDescription(
                    PythonLaunchDescriptionSource(PathJoinSubstitution([arm_bringup_dir, 'launch', 'urdf.launch.py'])),
                    launch_arguments={'model': model, 'gazebo': gazebo, 'use_mock_hardware': use_mock_hardware, 'arm': arm, 'old_arm': old_arm}.items(),
                )],
        ),
    ]


def generate_launch_description():
    arm_bringup_dir = FindPackageShare('arm_bringup')
    rover_description_dir = FindPackageShare('rover_description')

    declared_arguments = [   
        DeclareLaunchArgument(
            name='controllers',
            default_value=PathJoinSubstitution([arm_bringup_dir, 'params', 'controllers.yaml']),
            description='Absolute path to controller params file',
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
        DeclareLaunchArgument(
            name='arm',
            default_value='false',
            description='whether to launch arm',
        ),
        DeclareLaunchArgument(
            name='old_arm',
            default_value='True',
            description='whether to launch the old arm (on new armware)',
        ),
        DeclareLaunchArgument(
            name='use_mock_hardware',
            default_value='false',
            description='whether to use mock hardware for hardware interfaces',
        ),
        DeclareLaunchArgument(
            name='arm',
            default_value='true',
            description='whether to launch arm',
        ),
        DeclareLaunchArgument(
            name='old_arm',
            default_value='false',
            description='whether to launch old arm',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
