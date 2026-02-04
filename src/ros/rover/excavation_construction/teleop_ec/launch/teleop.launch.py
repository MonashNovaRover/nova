from launch import LaunchDescription
from launch.conditions import IfCondition, UnlessCondition
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, TextSubstitution, PythonExpression
from launch.actions import DeclareLaunchArgument, OpaqueFunction, GroupAction
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from launch_ros.parameter_descriptions import ParameterValue

def launch_setup(context, *args, **kwargs):
    log_inputs = LaunchConfiguration('log_inputs')
    joystick = LaunchConfiguration('joystick')
    log_level = LaunchConfiguration('log_level')

    params_dir = PathJoinSubstitution([FindPackageShare('teleop_ec'), 'params'])

    teleop_params = PathJoinSubstitution([params_dir, 'teleop.yaml'])
    joystick_params = PathJoinSubstitution([params_dir, "joysticks.config.yaml"])
    game_controller_params = PathJoinSubstitution([params_dir, "game_controller.config.yaml"])

    return [
        # using joysticks
        GroupAction(
            condition=IfCondition(joystick),
            actions=[
                Node(
                    package='teleop_node',
                    executable='teleop_node',
                    output='screen',
                    arguments=['--node-name', 'teleop_ec'],
                    parameters=[
                        teleop_params,
                        joystick_params,
                        {'log_inputs': ParameterValue(log_inputs, value_type=bool)}
                    ],
                    ros_arguments=['--log-level', log_level],
                    additional_env={
                        'RCUTILS_COLORIZED_OUTPUT': '1',
                    },
                ),
                Node(
                    name='joy_left',
                    package='joy',
                    executable='joy_node',
                    output='screen',
                    ros_arguments=['--log-level', log_level],
                    parameters=[{'device_id' : 0 ,}],
                    remappings=[('/joy', '/ec/joy/left')]
                ),
                Node(
                    name='joy_right',
                    package='joy',
                    executable='joy_node',
                    output='screen',
                    ros_arguments=['--log-level', log_level],
                    parameters=[{'device_id' : 1 ,}],
                    remappings=[('/joy', '/ec/joy/right')]
                )
            ]
        ),

        # using game controller
        GroupAction(
            condition=UnlessCondition(joystick),
            actions=[
                Node(
                    package='teleop_node',
                    executable='teleop_node',
                    output='screen',
                    arguments=['--node-name', 'teleop_ec'],
                    parameters=[
                        teleop_params,
                        game_controller_params,
                        {'log_inputs': ParameterValue(log_inputs, value_type=bool)}
                    ],
                    ros_arguments=['--log-level', log_level],
                    additional_env={
                        'RCUTILS_COLORIZED_OUTPUT': '1',
                    }
                ),
                Node(
                    package='joy',
                    executable='game_controller_node',
                    output='screen',
                    ros_arguments=['--log-level', log_level],
                    remappings=[('/joy', '/ec/joy')]
                )
            ]
        ),
    ]

def generate_launch_description():
    declared_arguments = [
        DeclareLaunchArgument(
            name='joystick',
            default_value='True',
            description='Use thrustmaster joysticks or a game controller',
        ),
        DeclareLaunchArgument(
            name='log_level',
            default_value='info',
            description='Log level of teleop node',
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