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
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, DeclareLaunchArgument, OpaqueFunction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
import subprocess

try:
    subprocess.run(["can", "start", "can1", "250000"], check=True)
    print("can1 started successfully (250000)")
except subprocess.CalledProcessError as e:
    print(f"Error: Failed to start can1.")
    print("{e}")

def launch_setup(context, *args, **kwargs):
    nova_bringup_dir = FindPackageShare('nova_bringup')
    
    scraper_card_type = LaunchConfiguration('scraper_card_type')

    return [
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
        ),
        IncludeLaunchDescription(
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([nova_bringup_dir, 'launch', 'drive.launch.py'])),
        ),
    ]

def generate_launch_description():
    nova_bringup_dir = FindPackageShare('nova_bringup')
    
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
