'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the rover to start all
   rover control scripts.

This only starts controllers for the path
    planner.

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
from launch.conditions import UnlessCondition
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction, GroupAction, ExecuteProcess, LogInfo
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from launch_ros.parameter_descriptions import ParameterValue


def launch_setup(context, *args, **kwargs):
    arm_bringup_dir = FindPackageShare('arm_bringup')

    angle = LaunchConfiguration('angle')
    controllers = LaunchConfiguration('controllers')
    gazebo = LaunchConfiguration('gazebo')
    log_level = LaunchConfiguration('log_level')
    model = LaunchConfiguration('model')
    arm = LaunchConfiguration('arm').perform(context)
    old_arm = LaunchConfiguration('old_arm').perform(context)
    use_local_mesh = LaunchConfiguration('use_local_mesh')
    use_mock_hardware = LaunchConfiguration('use_mock_hardware')

    return [
        Node( # TODO: only when arm is enabled
            package='controller_manager',
            executable='spawner',
            arguments=['nova_arm_position_controller']
        ),
        Node( # TODO: only when arm is enabled
            package='controller_manager',
            executable='spawner',
            arguments=['nova_path_planner'],
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
                    remappings=[('/controller_manager/robot_description', '/robot_description')],
                ),
                IncludeLaunchDescription(
                    PythonLaunchDescriptionSource(PathJoinSubstitution([arm_bringup_dir, 'launch', 'urdf.launch.py'])),
                    launch_arguments={'model': model, 'gazebo': gazebo, 'angle': angle, 'use_local_mesh': use_local_mesh, 'use_mock_hardware': use_mock_hardware, 'arm': arm, 'old_arm': old_arm}.items(),
                )],
        ),
    ]


def generate_launch_description():
    arm_bringup_dir = FindPackageShare('arm_bringup')
    rover_description_dir = FindPackageShare('rover_description')

    declared_arguments = [   
        DeclareLaunchArgument(
            name='angle', 
            default_value='15',
            description='Angle (in degrees) at which the camera is mounted',
        ),
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
            name='use_mock_hardware',
            default_value='false',
            description='whether to use mock hardware for hardware interfaces',
        ),
        DeclareLaunchArgument(
            name='use_local_mesh',
            default_value='False',
            description='Use local mesh paths instead of nix store paths',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
