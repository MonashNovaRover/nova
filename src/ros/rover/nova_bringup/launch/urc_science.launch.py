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
EDITED:     05/05/2025
EDITED BY: Tristan Clark, Josh Leivenzon, 
    Victor Bartlinski, Felicity Matthews,
    Brandon Chung
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
from launch import LaunchDescription
from launch.actions import OpaqueFunction, DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def launch_setup(context, *args, **kwargs):
    nova_bringup_dir = FindPackageShare('nova_bringup')
    hydroprobe_control_params = PathJoinSubstitution([nova_bringup_dir, 'params', 'hydroprobe_control.yaml'])

    # parameterised canIDs
    auger1_drill_canid = LaunchConfiguration('auger1_drill_canid')
    auger1_actuation_canid = LaunchConfiguration('auger1_actuation_canid')
    auger2_drill_canid = LaunchConfiguration('auger2_drill_canid')
    auger2_actuation_canid = LaunchConfiguration('auger2_actuation_canid')
    analysis_arm_cmd_canid = LaunchConfiguration('analysis_arm_cmd_canid')
    cbeam_actuation_canid = LaunchConfiguration('cbeam_actuation_canid')
    cache1_canid = LaunchConfiguration('cache1_canid')
    cache2_canid = LaunchConfiguration('cache2_canid')

    return [
        Node(
            name='AnalysisArm',
            package='science',
            executable='analysis_arm.py',
            output='screen',
            emulate_tty=True,
            parameters=[{
                "cmd_id": analysis_arm_cmd_canid,
                "active": True,
                "active_button": "btn_bottom_r1_state",
                "inactive_button_pool": ["btn_bottom_r4_state"],
                "using_left_joystick": True,
            }],
        ),
        Node(
            name='CBeam',
            package='science',
            executable='analysis_arm.py',
            output='screen',
            emulate_tty=True,
            parameters=[{
                "cmd_id": cbeam_actuation_canid,
                "active": False,
                "active_button": "btn_bottom_r4_state",
                "inactive_button_pool": ["btn_bottom_r1_state"],
                "using_left_joystick": True,
            }],
        ),
        Node(
            name='Auger1',
            package='science',
            executable='auger.py',
            output='screen',
            emulate_tty=True,
            parameters=[{
                "auger_drill_canid": auger1_drill_canid,
                "auger_actuation_canid": auger1_actuation_canid,
                "active": True,
                "active_button": "btn_bottom_r1_state",
                "inactive_button_pool": ["btn_bottom_r4_state"],
                "using_left_joystick": False,
            }]
        ),
        Node(
            name='Auger2',
            package='science',
            executable='auger.py',
            output='screen',
            emulate_tty=True,
            parameters=[{
                "auger_drill_canid": auger2_drill_canid,
                "auger_actuation_canid": auger2_actuation_canid,
                "active": False,
                "active_button": "btn_bottom_r4_state",
                "inactive_button_pool": ["btn_bottom_r1_state"],
                "using_left_joystick": False,
            }]
        ),
        Node(
            package='science',
            executable='urc_bme_sensor.py',
            output='screen',
            emulate_tty=True,
        ),
        Node(
            name="Cache1",
            package='science',
            executable='urc_cache.py',
            output='screen',
            emulate_tty=True,
            parameters=[{
                "frame_id": cache1_canid,
                "cache_id": "1",
                "servo_command": 0x01
            }],
        ),
        Node(
            name="Cache2",
            package='science',
            executable='urc_cache.py',
            output='screen',
            emulate_tty=True,
            parameters=[{
                "frame_id": cache2_canid,
                "cache_id": "2",
                "servo_command": 0x02
            }],
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
            name="HydroprobeControl",
            package='science',
            executable='urc_hydraprobe_control.py',
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
        DeclareLaunchArgument("auger1_drill_canid", default_value='0x0C1'),
        DeclareLaunchArgument("auger1_actuation_canid", default_value='0x0D2'),
        DeclareLaunchArgument("auger2_drill_canid", default_value='0x0C2'),
        DeclareLaunchArgument("auger2_actuation_canid", default_value='0x0D2'),
        DeclareLaunchArgument("cbeam_actuation_canid", default_value='0x041'),
        DeclareLaunchArgument("analysis_arm_cmd_canid", default_value='0x042'),
        DeclareLaunchArgument("cache1_canid", default_value='0x0B0'),
        DeclareLaunchArgument("cache2_canid", default_value='0x0B0'),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
