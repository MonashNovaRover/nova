'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	drone_gps
CREATION:	15/04/2026
EDITED:     15/04/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.substitutions import  PathJoinSubstitution, LaunchConfiguration, IfElseSubstitution
from launch_ros.substitutions import FindPackageShare

from os.path import expanduser

def launch_setup(context, *args, **kwargs):
    sim = LaunchConfiguration('sim')
    mavros_params = LaunchConfiguration('mavros_params')

    return [
        Node(
            package='mavros',
            executable='mavros_node',
            name='mavros',
            output='screen',
            parameters=[mavros_params, {'use_sim_time': sim}],
        ),
    ]


def generate_launch_description():
    local = LaunchConfiguration('local')

    drone_gps_dir = IfElseSubstitution(local,
        PathJoinSubstitution([expanduser("~") + '/nova/src/ros/rover/drone/drone_gps']),
        FindPackageShare('drone_gps')
    )

    declared_arguments = [
        DeclareLaunchArgument(
            name='local',
            default_value='False',
            description='Whether to use local directories instead of the nix store.',
        ),
        DeclareLaunchArgument(
            name='sim',
            default_value='True',
            description='Use /clock?',
        ),
        DeclareLaunchArgument(
            name='mavros_params',
            default_value=PathJoinSubstitution([drive_bringup_dir, 'params', 'mavros.yaml']),
            description='Absolute path to the drone params file',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
