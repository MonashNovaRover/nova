"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the rover to start all
   science scripts.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - science/nir_probe_publisher.py      [nir_probe_publisher]
  - science/microscope_servo.py         [microscope_servo]
  - science/kiln_server.py              [kiln_server]
  - control/auger.py                    [auger]
  - control/analysis_platform.py        [analysis_platform]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CREATION:   17/03/2024
EDITED:     13/02/2025
EDITED BY: Tristan Clark, Josh Leivenzon, 
    Victor Bartlinski, Felicity Matthews
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
from launch import LaunchDescription
from launch.actions import OpaqueFunction, DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

def launch_setup(context, *args, **kwargs):
    # parameterised canIDs
    auger_drill_canid = LaunchConfiguration('auger_drill_canid')
    auger_actuation_canid = LaunchConfiguration('auger_actuation_canid')
    analysis_arm_cmd_canid = LaunchConfiguration('analysis_arm_cmd_canid')
    tof_canid = LaunchConfiguration('tof_canid')

    return [
        Node(
            package='science',
            executable='urc_analysis_arm.py',
            output='screen',
            emulate_tty=True,
            parameters=[{
                "cmd_id": analysis_arm_cmd_canid,
                "tof_frame_id": tof_canid
            }],
        ),
        Node(
            package='science',
            executable='urc_auger.py',
            output='screen',
            emulate_tty=True,
            parameters=[{
                "auger_drill_canid": auger_drill_canid,
                "auger_actuation_canid": auger_actuation_canid
            }]
        ),
        Node(
            package='science',
            executable='urc_bme_sensor.py',
            output='screen',
            emulate_tty=True,
        ),
        Node(
            package='science',
            executable='urc_cache.py',
            output='screen',
            emulate_tty=True,
        ),
        Node(
            package='science',
            executable='urc_carousel.py',
            output='screen',
            emulate_tty=True,
        ),
        Node(
            package='science',
            executable='urc_heater.py',
            output='screen',
            emulate_tty=True,
        ),
        Node(
            package='science',
            executable='urc_hydraprobe.py',
            output='screen',
            emulate_tty=True,
        ),
        Node(
            package='science',
            executable='urc_mixers.py',
            output='screen',
            emulate_tty=True,
        ),
        Node(
            package='science',
            executable='urc_pumps.py',
            output='screen',
            emulate_tty=True,
        ),
        Node(
            package='science',
            executable='urc_raman_spec_server.py',
            output='screen',
            emulate_tty=True,
        ),
        Node(
            package='science',
            executable='urc_sample_tray.py',
            output='screen',
            emulate_tty=True,
        ),
        Node(
            package='science',
            executable='urc_theta_360_cam.py',
            output='screen',
            emulate_tty=True,
        ),
        Node(
            package='science',
            executable='urc_uv_vis_leds.py',
            output='screen',
            emulate_tty=True,
        ),
        Node(
            package='science',
            executable='urc_uv_vis_spec.py',
            output='screen',
            emulate_tty=True,
        ),
    ]

def generate_launch_description():
    declared_arguments = [
        DeclareLaunchArgument("auger_drill_canid", default_value='0x0C1'),
        DeclareLaunchArgument("auger_actuation_canid", default_value='0x0C2'),
        DeclareLaunchArgument("analysis_arm_cmd_canid", default_value='0x032'),
        DeclareLaunchArgument("tof_canid", default_value='0x456')
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
