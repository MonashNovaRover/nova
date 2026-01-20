'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the rover to start all
   excavation and construction scripts.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CREATION:   11/02/2024
EDITED:     04/02/2025
EDITED BY: Tristan Clark, Taaj Street, 
    Victor Bartlinski
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from datetime import datetime
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, DeclareLaunchArgument, OpaqueFunction, ExecuteProcess
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    scraper_card_type = LaunchConfiguration('scraper_card_type')

    timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    log_file = f"/home/nova/logs/{timestamp}_ec.txt"

    return [
        ExecuteProcess(
            cmd=[
                "bash",
                "-c",
                f"candump can0 > {log_file}",
            ],
            output="screen"
        ),
        Node(
            package='excavation_construction', 
            executable='scraper', 
            output='screen', 
            emulate_tty=True, 
            parameters=[{'card_type': scraper_card_type}],
        ),
        Node(
            package='excavation_construction', 
            executable='tile_placer', 
            output='screen', 
            emulate_tty=True,
        )
    ]

def generate_launch_description():
    declared_arguments = [ 
        DeclareLaunchArgument(
            name='scraper_card_type', 
            default_value='CMD', 
            description='Card type for the scraper node',
        ),     
    ]
    
    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )