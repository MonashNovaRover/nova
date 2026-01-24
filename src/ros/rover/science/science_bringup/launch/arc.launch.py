"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ARC launch file for science payload

[NEW] version - use teleop science with this.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - science/analysis_arm.py             [analysis_arm]
  - science/nir_probe_publisher.py      [nir_probe_publisher]
  - science/arc_sweeper_servo.py        [sweeper]
  - science/tool_rotator.py             [tool_rotator] (analysis arm)
  - science/chute.py                    [chute]
  - science/auger.py                    [auger]
  - science/analysis_arm.py             [c_beam]
  - science/kiln.py                     [kiln]
  - science/scimbal_cam.py              [scimbal_cam]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CREATED:    16/01/2026
EDITED:     16/01/2026
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
            name='tool_rotator',
            package='science',
            executable='tool_rotator.py',
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
            executable='auger.py',
            output='screen',
            emulate_tty=True,
            parameters=[
                science_params,
            ],
        ),
        Node(
            name='kiln',
            package='science',
            executable='kiln.py',
            output='screen',
            emulate_tty=True,
            parameters=[
                science_params,
            ],
        ),


        # Misc - Nodes for misc components
        Node(
            name='chute',
            package='science',
            executable='chute.py',
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
            default_value=PathJoinSubstitution([science_bringup_dir, 'params', 'arc.yaml']),
            description='The main parameter file to use for the science nodes',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
