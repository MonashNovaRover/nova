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
            package='science', executable='urc_analysis_arm.py', output='screen', emulate_tty=True),

        Node(
            package='science', executable='urc_auger.py', output='screen', emulate_tty=True),
        
        Node(
            package='science', executable='urc_carousel.py', output='screen', emulate_tty=True),

        Node(
            package='science', executable='urc_hydraprobe.py', output='screen', emulate_tty=True),

        Node(
            package='science', executable='urc_mixers.py', output='screen', emulate_tty=True),

        Node(
            package='science', executable='urc_pumps.py', output='screen', emulate_tty=True),

        Node(
            package='science', executable='urc_sample_tray.py', output='screen', emulate_tty=True),

        Node(
            package='science', executable='urc_theta_360_cam.py', output='screen', emulate_tty=True),

        Node(
            package='science', executable='urc_cache.py', output='screen', emulate_tty=True),

        Node(
            package='science', executable='urc_heater.py', output='screen', emulate_tty=True),
        # Node(
        #     package='science', executable='uv_vis_spec.py', output='screen', emulate_tty=True),

    ])
