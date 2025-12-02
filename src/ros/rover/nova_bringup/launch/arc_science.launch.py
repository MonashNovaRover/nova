"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the rover to start all
   science scripts.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - science/nir_probe_publisher.py      [nir_probe_publisher]
  - science/kiln_server.py              [kiln_server]
  - science/auger.py                    [auger]
  - science/analysis_arm.py             [AnalysisArm]
  - science/analysis_arm.py             [CBeam]
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
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, PythonExpression
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    science_params = LaunchConfiguration('science_params')

    return [
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
            name='kiln_server',
            package='science',
            executable='kiln_server.py',
            output='screen',
            emulate_tty=True,
            parameters=[
                science_params,
            ],
        ),
        Node(
            name='auger',
            package='science',
            executable='auger.py',
            output='screen',
            emulate_tty=True,
            parameters=[
                science_params,
            ],
        ),
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
    nova_bringup_dir = PythonExpression([
        '"', PathJoinSubstitution(['/home/nova/nova/src/ros/rover/nova_bringup/']),
        '" if "', LaunchConfiguration('local'), '".lower() == "true" else "',
        FindPackageShare('nova_bringup'), '"'
    ])

    declared_arguments = [
        # You can declare arguments to your launch file like this!
        DeclareLaunchArgument(
            name='local',
            default_value='False',
            description='Whether to use the local teleop_science source directory instead of the nix store for param files.',
        ),
        DeclareLaunchArgument(
            name='science_params',
            default_value=PathJoinSubstitution([nova_bringup_dir, 'params', 'arc_science.yaml']),
            description='The main parameter file to use for the arc science nodes',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
