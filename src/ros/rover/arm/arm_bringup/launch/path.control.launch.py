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
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, PythonExpression, Command
from launch.conditions import UnlessCondition
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction, GroupAction, ExecuteProcess, LogInfo
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from launch_ros.parameter_descriptions import ParameterValue
import os


def launch_setup(context, *args, **kwargs):
    arm_bringup_dir = PythonExpression([
        '"', PathJoinSubstitution([os.path.expanduser("~") + '/nova/src/ros/rover/arm/arm_bringup']),
        '" if bool("', LaunchConfiguration('local'), '") else "',
        FindPackageShare('arm_bringup'), '"'
    ])
    rover_description_dir = PythonExpression([
        '"', PathJoinSubstitution([os.path.expanduser("~") + '/nova/src/ros/rover/rover_description']),
        '" if bool("', LaunchConfiguration('local'), '") else "',
        FindPackageShare('rover_description'), '"'
    ])

    angle = LaunchConfiguration('angle')
    controllers = LaunchConfiguration('controllers')
    gazebo = LaunchConfiguration('gazebo')
    log_level = LaunchConfiguration('log_level')
    model = LaunchConfiguration('model')
    arm = LaunchConfiguration('arm').perform(context)
    old_arm = LaunchConfiguration('old_arm').perform(context)
    use_local_mesh = LaunchConfiguration('use_local_mesh')
    use_mock_hardware = LaunchConfiguration('use_mock_hardware')
    robot_name = LaunchConfiguration('robot_name')
    path_planner_controller_name = 'nova_path_planner'

    xacro_args = [
        'gazebo:=', gazebo, ' ',
        'robot_name:=', robot_name, ' ',
        'arm:=', arm, ' ',
        'old_arm:=', old_arm, ' ',
        'use_mock_hardware:=', use_mock_hardware, ' ',
        'auto_camera:=false ',
        'rover_description_dir:=', rover_description_dir, ' ',
        'drive_control:=False ', 'arm_control:=True '
    ]
    urdf_value = ParameterValue(Command(['xacro ', model, ' '] + xacro_args), value_type=str)

    show_colours_additional_env = {
        # Show colors in the terminal output
        'RCUTILS_COLORIZED_OUTPUT': '1',
        # (Optional!) omit time from the logs
        'RCUTILS_CONSOLE_OUTPUT_FORMAT': '[{severity}] [{name}] {message}',
    }


    return [
        LogInfo(msg=['Using arm_bringup := ', arm_bringup_dir]),
        LogInfo(msg=['Using model := ', model]),

        LogInfo(msg=['arm := "', arm, '"']),
        LogInfo(msg=['old_arm := "', old_arm, '"']),
        # Node( # TODO: only when arm is enabled
        #     package='controller_manager',
        #     executable='spawner',
        #     arguments=['nova_arm_position_controller']
        # ),
        Node( # TODO: only when arm is enabled
            package='controller_manager',
            executable='spawner',
            arguments=[path_planner_controller_name, '-c', '/arm/controller_manager'],
        ),
        Node(
            package='joint_state_publisher',
            executable='joint_state_publisher',
            parameters=[{
                'source_list': ['/arm/joint_states'],
                'publish_default_positions': True,
            }],
        ),
        Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            namespace='/arm',
            parameters=[{'robot_description': urdf_value}],
            additional_env=show_colours_additional_env,
        ),
        GroupAction(
            condition=UnlessCondition(gazebo),
            actions=[
                Node(
                    package='controller_manager',
                    executable='spawner',
                    arguments=['joint_state_broadcaster_arm', '-c', '/arm/controller_manager'],
                ),
                Node(
                    package='controller_manager',
                    executable='ros2_control_node',
                    namespace='/arm',
                    parameters=[controllers],
                    remappings=[('/arm/controller_manager/robot_description', '/arm/robot_description')],
                )
                # IncludeLaunchDescription(
                #     PythonLaunchDescriptionSource(PathJoinSubstitution([arm_bringup_dir, 'launch', 'urdf.launch.py'])),
                #     launch_arguments={'model': model, 'gazebo': gazebo, 'angle': angle, 'use_local_mesh': use_local_mesh, 'use_mock_hardware': use_mock_hardware, 'arm': arm, 'old_arm': old_arm}.items(),
                # )
            ],
        ),
    ]


def generate_launch_description():
    arm_bringup_dir = PythonExpression([
        '"', PathJoinSubstitution([os.path.expanduser("~") + '/nova/src/ros/rover/arm/arm_bringup']),
        '" if bool("', LaunchConfiguration('local'), '") else "',
        FindPackageShare('arm_bringup'), '"'
    ])
    rover_description_dir = PythonExpression([
        '"', PathJoinSubstitution([os.path.expanduser("~") + '/nova/src/ros/rover/rover_description']),
        '" if bool("', LaunchConfiguration('local'), '") else "',
        FindPackageShare('rover_description'), '"'
    ])

    declared_arguments = [   
        DeclareLaunchArgument(
            name='local',
            default_value='False',
            description='whether to use the local rover_description source directory instead of the nix store.',
        ),
        DeclareLaunchArgument(
            name='arm',
            default_value='true',
        ),
        DeclareLaunchArgument(
            name='old_arm',
            default_value='false',
        ),
        DeclareLaunchArgument(
            name='angle', 
            default_value='15',
            description='Angle (in degrees) at which the camera is mounted',
        ),
        DeclareLaunchArgument(
            name='controllers',
            default_value=PathJoinSubstitution([arm_bringup_dir, 'params', 'new.new.controllers.yaml']),
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
            name='robot_name',
            default_value='Banksia',
            description='name of the robot',
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
