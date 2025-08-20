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
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, PythonExpression, Command
from launch.conditions import IfCondition, UnlessCondition
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction, GroupAction, ExecuteProcess, LogInfo
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from launch_ros.parameter_descriptions import ParameterValue


def launch_setup(context, *args, **kwargs):
    arm_bringup_dir = PythonExpression([
        '"', PathJoinSubstitution(['/home/nova/nova/src/ros/rover/arm/arm_bringup']),
        '" if "', LaunchConfiguration('local'), '".lower() == "true" else "',
        FindPackageShare('arm_bringup'), '"'
    ])
    rover_description_dir = PythonExpression([
        '"', PathJoinSubstitution(['/home/nova/nova/src/ros/rover/rover_description']),
        '" if "', LaunchConfiguration('local'), '".lower() == "true" else "',
        FindPackageShare('rover_description'), '"'
    ])

    controllers = LaunchConfiguration('controllers')
    gazebo = LaunchConfiguration('gazebo')
    log_level = LaunchConfiguration('log_level')
    model = LaunchConfiguration('model')
    arm = LaunchConfiguration('arm').perform(context)
    old_arm = LaunchConfiguration('old_arm').perform(context)
    use_mock_hardware = LaunchConfiguration('use_mock_hardware')
    robot_name = LaunchConfiguration('robot_name')
    urdf = LaunchConfiguration('urdf')
    rviz = LaunchConfiguration('rviz').perform(context)

    show_colours_additional_env = {
        # Show colors in the terminal output
        'RCUTILS_COLORIZED_OUTPUT': '1',
        # (Optional!) omit time from the logs
        'RCUTILS_CONSOLE_OUTPUT_FORMAT': '[{severity}] [{name}] {message}',
    }

    xacro_args = [
        'gazebo:=', gazebo, ' ',
        'robot_name:=', robot_name, ' ',
        'arm:=', arm, ' ',
        'old_arm:=', old_arm, ' ',
        'use_mock_hardware:=', use_mock_hardware, ' ',
        'auto_camera:=false ',
        'rover_description_dir:=', rover_description_dir, ' ',
    ]
    urdf_value = ParameterValue(Command(['xacro ', model, ' '] + xacro_args), value_type=str)

    return [
        LogInfo(msg=[
            '"', PathJoinSubstitution(['/home/nova/nova/src/ros/rover/arm/arm_bringup']),
            '" if "', LaunchConfiguration('local'), '".lower() == "true" else "',
            FindPackageShare('arm_bringup'), '"'
        ]),
        LogInfo(msg=['Using arm_bringup := ', arm_bringup_dir]),
        LogInfo(msg=['Using model := ', model]),
        GroupAction(
            condition=IfCondition(PythonExpression([arm, " or ", old_arm])),
            actions=[
                Node(
                    package='controller_manager',
                    executable='spawner',
                    arguments=['nova_arm_velocity_controller', '--inactive'],
                    additional_env=show_colours_additional_env,
                ),
                Node(
                    package='controller_manager',
                    executable='spawner',
                    arguments=['nova_arm_position_controller', 'nova_twistmapper', '--inactive'],
                    additional_env=show_colours_additional_env,
                ),
            ]
        ),
        Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            parameters=[{'robot_description': urdf_value}],
            additional_env=show_colours_additional_env,
        ),
        GroupAction(
            condition=UnlessCondition(gazebo),
            actions=[
                Node(
                    package='controller_manager',
                    executable='spawner',
                    arguments=['joint_state_broadcaster'],
                    additional_env=show_colours_additional_env,
                ),
                Node(
                    package='controller_manager',
                    executable='ros2_control_node',
                    parameters=[controllers],
                    remappings=[('/controller_manager/robot_description', '/robot_description'), ('/joint_states', '/arm/joint_states')],
                    additional_env=show_colours_additional_env,
                ),
                IncludeLaunchDescription(
                    PythonLaunchDescriptionSource(PathJoinSubstitution([arm_bringup_dir, 'launch', 'urdf.launch.py'])),
                    launch_arguments={'model': model, 'gazebo': gazebo, 'use_mock_hardware': use_mock_hardware, 'arm': arm, 'old_arm': old_arm, 'rviz': rviz}.items(),
                    condition=IfCondition(urdf),
                )
            ],
        ),
    ]


def generate_launch_description():
    arm_bringup_dir = PythonExpression([
        '"', PathJoinSubstitution(['/home/nova/nova/src/ros/rover/arm/arm_bringup']),
        '" if bool("', LaunchConfiguration('local'), '") else "',
        FindPackageShare('arm_bringup'), '"'
    ])
    rover_description_dir = PythonExpression([
        '"', PathJoinSubstitution(['/home/nova/nova/src/ros/rover/rover_description']),
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
            name='controllers',
            default_value=PathJoinSubstitution([arm_bringup_dir, 'params', 'new.controllers.yaml']),
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
            default_value='True',
            description='whether to launch arm',
        ),
        DeclareLaunchArgument(
            name='old_arm',
            default_value='False',
            description='whether to launch the old arm (on new armware)',
        ),
        DeclareLaunchArgument(
            name='use_mock_hardware',
            default_value='False',
            description='whether to use mock hardware for hardware interfaces',
        ),
        DeclareLaunchArgument(
            name='robot_name',
            default_value='Banksia',
            description='name of the robot',
        ),
        DeclareLaunchArgument(
            name='urdf',
            default_value='True',
            description='whether to run urdf.launch.py to publish values to /tf (needed for rviz to work)',
        ),
        DeclareLaunchArgument(
            name='rviz',
            default_value='False',
            description='whether to run rviz2 to visualize the robot (urdf must also be True).',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
