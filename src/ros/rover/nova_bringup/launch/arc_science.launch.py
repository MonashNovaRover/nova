'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the rover to start all
   science scripts.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - science/nir_probe_publisher.py      [nir_probe_publisher]
  - science/microscope_servo.py         [microscope_servo]
  - science/kiln_server.py'             [kiln_server]
  - control/auger.py                    [auger]
  - control/analysis_platform.py        [analysis_platform]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CREATED:    17/03/2024
EDITED:     01/01/2025
EDITED BY: Tristan Clark, Victor Bartlinski
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from launch import LaunchDescription
from launch.actions import OpaqueFunction
from launch_ros.actions import Node

def launch_setup(context, *args, **kwargs):
    return [
        Node(
            package='science', 
            executable='nir_probe_publisher.py', 
            output='screen', 
            emulate_tty=True,
        ),
        Node(
            package='science', 
            executable='microscope_servo.py', 
            output='screen', 
            emulate_tty=True,
        ),
        Node(
            package='science', 
            executable='kiln_server.py', 
            output='screen', 
            emulate_tty=True,
        ),
        Node(
            package='science', 
            executable='auger.py', 
            output='screen', 
            emulate_tty=True,
        ),
        Node(
            package='science', 
            executable='analysis_platform.py', 
            output='screen', 
            emulate_tty=True,
        ),
    ]

def generate_launch_description():
    return LaunchDescription([OpaqueFunction(function=launch_setup)])
