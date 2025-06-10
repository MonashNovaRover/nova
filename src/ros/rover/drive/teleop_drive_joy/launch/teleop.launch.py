from launch import LaunchDescription
from launch.actions import OpaqueFunction, DeclareLaunchArgument, LogInfo
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def launch_setup(context, *args, **kwargs):
    teleop_drive_joy_dir = FindPackageShare('teleop_drive_joy')

    # Launch configurations
    device_id = LaunchConfiguration('device_id')
    device_name = LaunchConfiguration('device_name')
    joy_vel = LaunchConfiguration('joy_vel')
    config = LaunchConfiguration('config').perform(context)
    params_file = PathJoinSubstitution([teleop_drive_joy_dir, 'config', f'{config}.config.yaml'])

    return [
        # Add Nodes
        Node(
            package='joy',
            executable='joy_node',
            name='joy_node',
            parameters=[
                {'device_id': device_id,
                 'deadzone': 0.1,
                 'autorepeat_rate': 20.0} | {'device_name': device_name} if device_name else {}
            ],
        ),
        Node(
            package='teleop_drive_joy',
            executable='teleop_drive_joy_node',
            name='teleop_drive_joy_node',
            parameters=[params_file],
            remappings=[
                ('/cmd_vel', joy_vel),
            ],
        ),
        # Log Information
        LogInfo(msg=['Joystick Loaded: ', config]),
    ]

def generate_launch_description():
    declared_arguments = [
        DeclareLaunchArgument(
            name='device_id',
            default_value='1',
        ),
        DeclareLaunchArgument(
            name='device_name',
            default_value='',
        ),
        DeclareLaunchArgument(
            name='joy_vel', 
            default_value='cmd_vel'
        ),
        DeclareLaunchArgument(
            name='config',
            default_value='nintendo', #Defined in launch_setup due to requiring another launch argument
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )