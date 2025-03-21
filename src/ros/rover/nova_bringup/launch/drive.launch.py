'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the rover to start all
   rover control scripts.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - control/drive/drive_inputs      [drive_cmd]
  - control/drive/driver            [driver]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CREATION:   15/12/2021
EDITED:     19/03/2025
EDITED BY: Max Tory, Taaj Street, 
    Victor Bartlinski, Jared Landau
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
import subprocess

try:
    subprocess.run(["can", "start", "can0", "250000"], check=True)
    print("can0 started successfully")
except subprocess.CalledProcessError as e:
    print(f"Error: Failed to start can0.")
    print("{e}")
    exit(1)

def launch_setup(context, *args, **kwargs):
    gazebo = LaunchConfiguration('gazebo')

    return [
        Node(
            package='drive', 
            executable='drive_inputs', 
            output='screen', 
            emulate_tty=True,
            parameters=[{'use_sim_time': gazebo}],
        ),
        Node(
            package='drive', 
            executable='driver', 
            output='screen', 
            emulate_tty=True,
            parameters=[{'use_sim_time': gazebo, 'gazebo': gazebo}],
        ),
        Node(
            package='blcmd_utils', 
            executable='status_monitor', 
            output='screen', 
            emulate_tty=True,
        ),
    ]

def generate_launch_description():
    declared_arguments = [
        DeclareLaunchArgument(
            name='gazebo', 
            default_value='False',
            description='',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
