"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
URC launch file for science payload

[NEW] version - use teleop science with this.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - science/nir_probe_publisher.py      [nir_probe_publisher]
  - science/chute.py                    [chute]
  - science/auger.py                    [auger]
  - science/analysis_arm.py             [c_beam]
  - science/scimbal_cam.py              [scimbal_cam]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CREATED:    05/04/26
EDITED:     05/04/26
EDITED BY:  Felicity Matthews
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
from launch import LaunchDescription
from launch.actions import OpaqueFunction, DeclareLaunchArgument, IncludeLaunchDescription
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, PythonExpression
from launch_ros.actions import Node
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    nova_bringup_dir = PythonExpression([
        '"', PathJoinSubstitution(['/home/nova/nova/src/ros/rover/nova_bringup/']),
        '" if "', LaunchConfiguration('local'), '".lower() == "true" else "',
        FindPackageShare('nova_bringup'), '"'
    ])

    science_params = LaunchConfiguration('science_params')


    return [
        # Analysis Arm - Nodes for components on the analysis arm
        Node(
            name='analysis_arm',
            package='science',
            executable='cbeam.py',
            output='screen',
            emulate_tty=True,
            parameters=[
                science_params,
            ],
        ),
        # Node(
        #     name='time_of_flight_sensor',
        #     package='science',
        #     executable='time_of_flight.py',
        #     output='screen',
        #     emulate_tty=True,
        #     parameters=[
        #         science_params
        #     ],
        # ),
        Node(
            name='nir_probe',
            package='science',
            executable='nir_probe.py',
            output='screen',
            emulate_tty=True,
            parameters=[science_params],
        ),

        # CBeam - Nodes for components on the CBeam
        Node(
            name='auger_left',
            package='science',
            executable='auger.py',
            output='screen',
            emulate_tty=True,
            parameters=[
                science_params,
            ],
        ),
        Node(
            name='auger_right',
            package='science',
            executable='auger.py',
            output='screen',
            emulate_tty=True,
            parameters=[
                science_params,
            ],
        ),
        Node(
            name='cbeam',
            package='science',
            executable='cbeam.py',
            output='screen',
            emulate_tty=True,
            parameters=[
                science_params,
            ],
        ),


        # Science Belly - Nodes for components in the belly
        Node(
            name='pumps',
            package='science',
            executable='pumps.py',
            output='screen',
            emulate_tty=True,
            parameters=[
                science_params,
            ],
        ),
        Node(
            name='carousel_inner',
            package='science',
            executable='carousel.py',
            output='screen',
            emulate_tty=True,
            parameters=[
                science_params,
            ],
        ),
        Node(
            name='carousel_outer',
            package='science',
            executable='carousel.py',
            output='screen',
            emulate_tty=True,
            parameters=[
                science_params,
            ],
        ),
        Node(
            name='heater',
            package='science',
            executable='heater.py',
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
        Node(
            name='power_cycle',
            package='science',
            executable='power_cycle.py',
            output='screen',
            emulate_tty=True,
            parameters=[
                science_params,
            ],
        ),
        Node(
            name='urc_bme_sensor',
            package='science',
            executable='urc_bme_sensor.py',
            output='screen',
            emulate_tty=True,
            parameters=[
                science_params,
            ],
        ),

        # launch CAN bus
        IncludeLaunchDescription(
            launch_description_source=PythonLaunchDescriptionSource(
                PathJoinSubstitution([nova_bringup_dir, "launch", "can.launch.py"])
            ),
            launch_arguments={
                "bus" : "can1",
                "bitrate" : "250000",
                "log_name" : "science-urc",
            }.items()
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
            default_value=PathJoinSubstitution([science_bringup_dir, 'params', 'urc.yaml']),
            description='The main parameter file to use for the science nodes',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
