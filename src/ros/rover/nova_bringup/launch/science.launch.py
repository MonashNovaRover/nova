"""
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
PACKAGE: 	core
CREATION:	17/03/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

# Include the required launch parameters
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        Node(
            package='science', executable='nir_probe_publisher.py', output='screen', emulate_tty=True),

        Node(
            package='science', executable='microscope_servo.py', output='screen', emulate_tty=True),

        Node(
            package='science', executable='kiln_server.py', output='screen', emulate_tty=True),

        Node(
            package='science', executable='auger.py', output='screen', emulate_tty=True),

        Node(
            package='science', executable='analysis_platform.py', output='screen', emulate_tty=True),
    ])
