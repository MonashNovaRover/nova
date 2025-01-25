
from launch import LaunchDescription
from launch.actions import OpaqueFunction, DeclareLaunchArgument, LogInfo
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def launch_setup(context, *args, **kwargs):
    teleop_arm_joy_dir = FindPackageShare('teleop_arm_joy')

    # Launch configurations
    joystick = LaunchConfiguration('joystick').perform(context)
    joy_device_L = LaunchConfiguration('joy_dev_L')
    joy_device_R = LaunchConfiguration('joy_dev_R')
    joy_vel = LaunchConfiguration('joy_vel')
    params_file = LaunchConfiguration(
        'params_file',
        default=PathJoinSubstitution([teleop_arm_joy_dir, 'config', f'{joystick}.config.yaml']),
    )

    return [
        # Add Nodes
        Node(
            package='joy',
            executable='joy_node',
            name='joy_node',
            parameters=[
                {'dev': joy_device_L,
                 'deadzone': 0.1,
                 'autorepeat_rate': 20.0}
            ],
            remappings=[
                ('__node', '/joy_node_L'),
                ('joy', 'joy_L')
            ]
        ),
        Node(
            package='joy',
            executable='joy_node',
            name='joy_node',
            parameters=[
                {'dev': joy_device_R,
                 'deadzone': 0.1,
                 'autorepeat_rate': 20.0}
            ],
            remappings=[
                ('__node', '/joy_node_R'),
                ('joy', 'joy_R')
            ]
        ),
        Node(
            package='teleop_arm_joy',
            executable='teleop_arm_joy_node',
            name='teleop_arm_joy_node',
            parameters=[PathJoinSubstitution([teleop_arm_joy_dir, 'config', f'{joystick}.config.yaml'])],
            remappings=[
                ('/cmd_vel', joy_vel),
            ],
        ),
        # Log Information
        LogInfo(msg=['Joystick Loaded: ', joystick]),
    ]

def generate_launch_description():
    declared_arguments = [
        DeclareLaunchArgument(
            name='joystick',
            default_value='thrustmasters',
        ),
        DeclareLaunchArgument(
            name='joy_dev_L',
            default_value='/dev/input/js0',
        ),
        DeclareLaunchArgument(
            name='joy_dev_R',
            default_value='/dev/input/js1',
        ),
        DeclareLaunchArgument(
            name='joy_vel',
            default_value='cmd_vel'
        ),
        DeclareLaunchArgument(
            name='params_file',
            default_value='', #Defined in launch_setup due to requiring another launch argument
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
