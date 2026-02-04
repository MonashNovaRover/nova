'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the rover to start all
   excavation and construction scripts.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - excavation_construction/scraper.py      [scraper]
  - excavation_construction/tile_placer.py  [tile_placer]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CREATION:   11/02/2024
EDITED:     04/02/2026
EDITED BY: Tristan Clark, Taaj Street, 
    Victor Bartlinski, Jonathan Jia
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from datetime import datetime
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, DeclareLaunchArgument, OpaqueFunction, ExecuteProcess
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, PythonExpression
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    nova_bringup_dir = FindPackageShare('nova_bringup')
    ec_params = LaunchConfiguration('ec_params')
    log_level = LaunchConfiguration('log_level')

    timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    log_file = f"/home/nova/logs/{timestamp}_ec.txt"

    return [
        IncludeLaunchDescription(
            launch_description_source=PythonLaunchDescriptionSource(
                PathJoinSubstitution([nova_bringup_dir, "launch", "can.launch.py"])
            ),
            launch_arguments={
                "bus" : "can0",
                "bitrate" : "250000",
                "log_name" : "ec",
            }.items()
        ),
        Node(
            package='excavation_construction', 
            executable='scraper', 
            output='screen', 
            emulate_tty=True, 
            parameters=[ec_params],
            ros_arguments=['--log-level', log_level],
        ),
        Node(
            package='excavation_construction', 
            executable='tile_placer', 
            output='screen', 
            emulate_tty=True,
            parameters=[ec_params],
            ros_arguments=['--log-level', log_level],
        ),
    ]

def generate_launch_description():
    nova_bringup_dir = PythonExpression([
        '"', PathJoinSubstitution(['/home/nova/nova/src/ros/rover/nova_bringup/']),
        '" if "', LaunchConfiguration('local'), '".lower() == "true" else "',
        FindPackageShare('nova_bringup'), '"'
    ])
    
    declared_arguments = [
        DeclareLaunchArgument(
            name='local',
            default_value='False',
            description='Whether to use the local source directory instead of the nix store for param files.',
        ),
        DeclareLaunchArgument(
            name='ec_params',
            default_value=PathJoinSubstitution([nova_bringup_dir, 'params', 'ec.yaml']),
            description='Parameter file passed to all ec nodes',
        ),
        DeclareLaunchArgument(
            name='log_level',
            default_value='info',
            description='Log level of launched nodes and launch files'
        )
    ]
    
    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )