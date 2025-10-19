from launch import LaunchDescription
from launch.actions import OpaqueFunction, DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def launch_setup(context, *args, **kwargs):

    # Launch configurations
    device_id = LaunchConfiguration('device_id')
    device_name = LaunchConfiguration('device_name')
    joy_vel = LaunchConfiguration('joy_vel')

    return [
        # Add Nodes
        Node(
            package='joy',
            executable='game_controller_node',
            name='game_controller_node',
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
            remappings=[
                ('/cmd_vel', joy_vel),
            ],
        ),
    ]

def generate_launch_description():
    declared_arguments = [
        DeclareLaunchArgument(
            name='device_id',
            default_value='0',
        ),
        DeclareLaunchArgument(
            name='device_name',
            default_value='',
        ),
        DeclareLaunchArgument(
            name='joy_vel', 
            default_value='cmd_vel'
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )