"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the rover to start all
   science scripts.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - science/nir_probe_publisher.py      [nir_probe_publisher]
  - science/kiln_server.py              [kiln_server]
  - science/urc_auger.py                [auger]
  - science/urc_analysis_arm.py         [analysis_arm]
  - science/arc_sweeper_servo.py        [sweeper]
  - science/arc_spinny_part.py          [spinny part] (analysis arm)
  - science/scimbal_cam.py              [scimbal_cam]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CREATED:    17/03/2024
EDITED:     20/03/2025
EDITED BY: Tristan Clark, Victor Bartlinski, Felicity Matthews
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
from launch import LaunchDescription
from launch.actions import OpaqueFunction, DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
import subprocess

try:
    subprocess.run(["can", "start", "can1", "250000"], check=True)
    print("can1 started successfully (250000)")
except subprocess.CalledProcessError as e:
    print(f"Error: Failed to start can1.")
    print("{e}")
    exit(1)

def launch_setup(context, *args, **kwargs):
    # parameterised canIDs
    auger_drill_canid = LaunchConfiguration('auger_drill_canid')
    auger_actuation_canid = LaunchConfiguration('auger_actuation_canid')
    analysis_arm_cmd_canid = LaunchConfiguration('analysis_arm_cmd_canid')
    tof_canid = LaunchConfiguration('tof_canid')

    return [
        Node(
            package='science',
            executable='nir_probe_publisher.py',
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
            executable='arc_sweeper_servo.py',
            output='screen',
            emulate_tty=True,
        ),
        Node(
            package='science',
            executable='arc_spinny_part.py',
            output='screen',
            emulate_tty=True,
        ),
        Node(
            package='science',
            executable='scimbal_cam.py',
            output='screen',
            emulate_tty=True,
        ),
    ]

def generate_launch_description():
    declared_arguments = [
        DeclareLaunchArgument("auger_drill_canid", default_value='0x0C1'),
        DeclareLaunchArgument("auger_actuation_canid", default_value='0x0D1'),
        DeclareLaunchArgument("analysis_arm_cmd_canid", default_value='0x0D2'),
        DeclareLaunchArgument("tof_canid", default_value='0x456')
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
