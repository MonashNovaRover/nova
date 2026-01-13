# teleop.launch.py
from launch import LaunchDescription
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, PythonExpression
from launch.actions import DeclareLaunchArgument, OpaqueFunction, GroupAction, LogInfo
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from launch_ros.parameter_descriptions import ParameterValue
from launch.conditions import IfCondition, UnlessCondition


def launch_setup(context, *args, **kwargs):
    teleop_science_dir = PythonExpression([
        '"', PathJoinSubstitution(['/home/nova/nova/src/ros/rover/science/teleop_science']),
        '" if "', LaunchConfiguration('local'), '".lower() == "true" else "',
        FindPackageShare('teleop_science'), '"'
    ])

    teleop_params = LaunchConfiguration('teleop_params')
    log_inputs = LaunchConfiguration('log_inputs')
    log_level = LaunchConfiguration('log_level').perform(context)
    use_joysticks = LaunchConfiguration('joystick').perform(context)
    comp = LaunchConfiguration('comp')

    input_param_file = PythonExpression([
        '"joysticks.config.yaml" if "', use_joysticks, '".lower() == "true" else "game_controller.config.yaml"'
    ])
    input_params = PathJoinSubstitution([teleop_science_dir, 'params', comp, input_param_file])

    return [
        LogInfo(msg=['Using teleop_science := ', teleop_science_dir]),
        LogInfo(msg=['Using comp := ', comp]),
        LogInfo(msg=['Using input parameters := ', input_params.perform(context)]),

        # Automatically run joy alongside teleop
        Node(
            condition=UnlessCondition(use_joysticks),
            package='joy',
            executable='game_controller_node',  # or joy_node
            output="screen"
        ),

        GroupAction(
            condition=IfCondition(use_joysticks),
            actions=[
                Node(
                    name="joy_left",
                    package='joy',
                    executable='joy_node',
                    output="screen",
                    parameters=[
                        {"device_id": 0, },
                    ],
                    remappings=[
                        ("/joy", "/joy_left")
                    ],
                ),
                Node(
                    name="joy_right",
                    package='joy',
                    executable='joy_node',
                    output="screen",
                    parameters=[
                        {"device_id": 1, },
                    ],
                    remappings=[
                        ("/joy", "/joy_right")
                    ],
                ),
            ],
        ),

        # Runs teleop_node with the given parameter files
        # This is the important part!
        Node(
            package='teleop_node',
            executable='teleop_node',
            output='screen',

            arguments=['--node-name', 'teleop_science', '--ros-args', '--log-level', log_level],

            # You can add multiple parameter files here:
            parameters=[
                teleop_params,
                input_params,
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
    teleop_science_dir = PythonExpression([
        '"', PathJoinSubstitution(['/home/nova/nova/src/ros/rover/science/teleop_science']),
        '" if "', LaunchConfiguration('local'), '".lower() == "true" else "',
        FindPackageShare('teleop_science'), '"'
    ])

    declared_arguments = [
        # You can declare arguments to your launch file like this!
        DeclareLaunchArgument(
            name='local',
            default_value='False',
            description='Whether to use the local teleop_science source directory instead of the nix store for param files.',
        ),
        DeclareLaunchArgument(
            name='log_level',
            default_value='INFO',
            description='The log level to use for the teleop_node',
        ),
        DeclareLaunchArgument(
            name='teleop_params',
            default_value=PathJoinSubstitution([teleop_science_dir, 'params', 'arc/teleop.yaml']),
            description='The main parameter file to use for the teleop_node',
        ),
        DeclareLaunchArgument(
            name='joystick',
            default_value="True",
            description='Whether to use joystick inputs or game controller joy source',
        ),
        DeclareLaunchArgument(
            name='comp',
            default_value="arc",
            description='What comp we are launching teleop for, options are "arc" or "urc"',
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