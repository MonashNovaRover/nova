from launch import LaunchDescription
from launch.actions import OpaqueFunction, DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PythonExpression, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from os.path import expanduser

def launch_setup(context, *args, **kwargs):

    # Launch configurations
    device_id = LaunchConfiguration('device_id')
    device_name = LaunchConfiguration('device_name')
    joy_vel = LaunchConfiguration('joy_vel')
    teleop_params = LaunchConfiguration('teleop_params')

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
            remappings=[
                ('/joy', '/drive/joy'),
                ('/joy/set_feedback', '/drive/joy/set_feedback'),
            ],
        ),
        Node(
            package='teleop_drive_joy',
            executable='teleop_drive_joy_node',
            name='teleop_drive_joy_node',
            parameters=[teleop_params],
            remappings=[
                ('/cmd_vel', joy_vel),
            ],
        ),
    ]

def generate_launch_description():
    teleop_drive_dir = PythonExpression([
        '"', PathJoinSubstitution([expanduser("~") + '/nova/src/ros/rover/drive/teleop_drive_joy']),
        '" if "', LaunchConfiguration('local'), '".lower() == "true" else "',
        FindPackageShare('teleop_drive_joy'), '"'
    ])

    declared_arguments = [
        DeclareLaunchArgument(
            name='local',
            default_value='False',
            description='Whether to use the local teleop_drive_joy source directory instead of the nix store for param files.',
        ),
        DeclareLaunchArgument(
            name='teleop_params',
            default_value=PathJoinSubstitution([teleop_drive_dir, 'params', 'teleop.yaml']),
            description='The main parameter file to use for teleop_drive_joy_node',
        ),
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