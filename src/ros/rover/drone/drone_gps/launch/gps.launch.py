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
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

from os.path import expanduser

def launch_setup(context, *args, **kwargs):
    sim = LaunchConfiguration('sim')

    return [
        Node(
            package='drone_gps',
            executable='gps.py',
            name='drone_gps',
            output='screen',
            parameters=[{'use_sim_time': sim}],
        ),
    ]


def generate_launch_description():

    declared_arguments = [
        DeclareLaunchArgument(
            name='sim',
            default_value='True',
            description='Use /clock?',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
