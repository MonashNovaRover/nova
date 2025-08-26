# teleop.launch.py
from launch import LaunchDescription
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, PythonExpression
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from launch_ros.parameter_descriptions import ParameterValue


def launch_setup(context, *args, **kwargs):
    teleop_drive_dir = PythonExpression([
        '"', PathJoinSubstitution(['/home/nova/nova/src/ros/rover/drive/teleop_drive']),
        '" if "', LaunchConfiguration('local'), '".lower() == "true" else "',
        FindPackageShare('teleop_drive'), '"'
    ])

    teleop_params = LaunchConfiguration('teleop_params')
    log_inputs = LaunchConfiguration('log_inputs')

    return [
        # Automatically run joy alongside teleop
        Node(
            package='joy',
            executable='game_controller_node',  # or joy_node
            output="screen"
        ),

        # Runs teleop_node with the given parameter files
        # This is the important part!
        Node(
            package='teleop_node',
            executable='teleop_node',
            output='screen',

            arguments=['--node-name', 'teleop_drive'],

            # You can add multiple parameter files here:
            parameters=[
                teleop_params,
                {'log_inputs': ParameterValue(log_inputs, value_type=bool)}
            ],

            additional_env={
                # Show colors in the terminal output
                'RCUTILS_COLORIZED_OUTPUT': '1',
                # (Optional!) omit time from the logs
                'RCUTILS_CONSOLE_OUTPUT_FORMAT': '[{severity}] [{name}] {message}',
            }
        ),
    ]


def generate_launch_description():
    teleop_drive_dir = PythonExpression([
        '"', PathJoinSubstitution(['/home/nova/nova/src/ros/rover/drive/teleop_drive']),
        '" if "', LaunchConfiguration('local'), '".lower() == "true" else "',
        FindPackageShare('teleop_drive'), '"'
    ])

    declared_arguments = [
        # You can declare arguments to your launch file like this!
        DeclareLaunchArgument(
            name='local',
            default_value='False',
            description='Whether to use the local teleop_drive source directory instead of the nix store for param files.',
        ),
        DeclareLaunchArgument(
            name='teleop_params',
            default_value=PathJoinSubstitution([teleop_drive_dir, 'params', 'teleop.yaml']),
            description='The main parameter file to use for the teleop_node',
        ),
        DeclareLaunchArgument(
            name='log_inputs',
            default_value='False',
            description='Set this true to display all the inputs. Very useful when trying to configure input sources!',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )