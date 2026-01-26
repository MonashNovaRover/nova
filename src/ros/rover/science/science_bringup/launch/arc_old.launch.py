"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ARC launch file for science payload

[OLD] version - must use launch-base with this
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - science/analysis_arm.py             [analysis_arm]
  - science/nir_probe_publisher.py      [nir_probe_publisher]
  - science/arc_sweeper_servo.py        [sweeper]
  - science/arc_spinny_part.py          [spinny part] (analysis arm)
  - science/auger.py                    [auger]
  - science/analysis_arm.py             [c_beam]
  - science/kiln_old.py                 [kiln]
  - science/scimbal_cam.py              [scimbal_cam]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CREATED:    12/01/2026
EDITED:     12/01/2026
EDITED BY:  Felicity Matthews
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
from launch import LaunchDescription
from launch.actions import OpaqueFunction, DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, PythonExpression
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    science_params = LaunchConfiguration('science_params')

    return [
        # Analysis Arm - Nodes for components on the analysis arm
        Node(
            name='analysis_arm',
            package='science',
            executable='analysis_arm.py',
            output='screen',
            emulate_tty=True,
            parameters=[
                science_params,
            ],
        ),
        Node(
            name='nir_probe_publisher',
            package='science',
            executable='nir_probe_publisher.py',
            output='screen',
            emulate_tty=True,
            parameters=[
                science_params,
            ],
        ),
        Node(
            name='arc_sweeper_servo',
            package='science',
            executable='arc_sweeper_servo.py',
            output='screen',
            emulate_tty=True,
            parameters=[
                science_params,
            ],
        ),
        Node(
            name='arc_spinny_part',
            package='science',
            executable='arc_spinny_part.py',
            output='screen',
            emulate_tty=True,
            parameters=[
                science_params,
            ],
        ),

        # CBeam - Nodes for components on the CBeam
        Node(
            name='auger',
            package='science',
            executable='auger_old.py',
            output='screen',
            emulate_tty=True,
            parameters=[
                science_params,
            ],
        ),
        Node(
            name='c_beam',
            package='science',
            executable='analysis_arm.py',
            output='screen',
            emulate_tty=True,
            parameters=[
                science_params,
            ],
        ),
        Node(
            name='kiln',
            package='science',
            executable='kiln_old.py',
            output='screen',
            emulate_tty=True,
            parameters=[
                science_params,
            ],
        ),

        # Misc - Nodes for misc components
        Node(
            name='scimbal_cam',
            package='science',
            executable='scimbal_cam.py',
            output='screen',
            emulate_tty=True,
            parameters=[
                science_params,
            ],
        ),
    ]

def generate_launch_description():
    science_bringup_dir = PythonExpression([
        '"', PathJoinSubstitution(['/home/nova/nova/src/ros/rover/science_bringup/']),
        '" if "', LaunchConfiguration('local'), '".lower() == "true" else "',
        FindPackageShare('science_bringup'), '"'
    ])

    declared_arguments = [
        # You can declare arguments to your launch file like this!
        DeclareLaunchArgument(
            name='local',
            default_value='False',
            description='Whether to use the local source directory instead of the nix store for param files.',
        ),
        DeclareLaunchArgument(
            name='science_params',
            default_value=PathJoinSubstitution([science_bringup_dir, 'params', 'arc_old.yaml']),
            description='The main parameter file to use for the science nodes',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
