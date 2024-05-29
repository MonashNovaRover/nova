"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the rover to start all
   arm control scripts.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - control/arm/arm_inputs              [arm_inputs]
  - control/arm/arm_kinematics          [arm_kinematics]
  - control/arm/arm_driver              [arm_driver]
  - electronics/electronics             [resolver_publisher.py]
  - visualisation/arm_viz_publisher     [arm_viz_publisher]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	core
CREATION:	17/12/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

# Include the required launch parameters
from launch import LaunchDescription
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from ament_index_python.packages import get_package_share_path
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch.conditions import IfCondition, UnlessCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource

# Generate the launch file with all inputs
def generate_launch_description():
    # Get the path to the parameters file in core/params
    arm_params = PathJoinSubstitution([FindPackageShare('nova_bringup'), 'params', 'arm_params.yaml'])
    description_dir = FindPackageShare('rover_description')

    sim = LaunchConfiguration('sim')
    urdf = LaunchConfiguration('urdf')
    urdf_path = LaunchConfiguration('urdf_path')
    rover_urdf = LaunchConfiguration('rover_urdf')
    namespace = LaunchConfiguration('namespace')
    
    return LaunchDescription([      
        DeclareLaunchArgument(
            'sim', 
            default_value='False'
        ),

        DeclareLaunchArgument(
            'arm_urdf', 
            default_value='True',
            description="Include arm URDF in robot_description?"
        ),

        DeclareLaunchArgument(
            'rover_urdf', 
            default_value='True',
            description="Include rover URDF in robot_description?"
        ),
                              
        DeclareLaunchArgument(
            'urdf', 
            default_value='False',
            description="Publish robot_description?"
        ),

        DeclareLaunchArgument(
            'arm_urdf_path', 
            default_value = PathJoinSubstitution(
                [
                    description_dir, 
                    'urdf', 
                    'arm.urdf.xacro'
                ]
            ), 
        ),

        DeclareLaunchArgument(
            'namespace', 
            default_value = '',
        ),

        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                PathJoinSubstitution(
                    [
                        FindPackageShare('nova_bringup'),
                        'launch',
                        'urdf.launch.py'
                    ]
                )
            ),
            launch_arguments = {
                "arm_urdf_path": LaunchConfiguration('arm_urdf_path'),
                "arm": LaunchConfiguration('arm_urdf'),
                "rover": LaunchConfiguration('rover_urdf')
            }.items(),
            condition=IfCondition(urdf)
        ),

        Node(
            package='arm', 
            executable='arm_inputs', 
            namespace=namespace,
            output='screen', 
            emulate_tty=True
        ),

        Node(
            package='arm', 
            executable='arm_twistmapper', 
            namespace=namespace,
            output='screen', 
            emulate_tty=True
        ),

        Node(
            package='arm', 
            executable='arm_control', 
            namespace=namespace,
            output='screen', 
            emulate_tty=True
        ),

        Node(
            package='arm', 
            executable='arm_driver', 
            namespace=namespace,
            output='screen', 
            emulate_tty=True, 
            condition=UnlessCondition(sim)
        ),

        Node(
            package='arm', 
            executable='resolver_publisher.py', 
            namespace=namespace,
            condition=UnlessCondition(sim), 
            parameters=[arm_params],
            output='screen', 
            emulate_tty=True
        ),

        Node(
            package='arm', 
            executable='arm_rviz_publisher', 
            namespace=namespace,
            output='screen', 
            emulate_tty=True
        ),

        Node(
            package='cmd_utils', 
            executable='CMD_publisher.py', 
            namespace=namespace,
            condition=UnlessCondition(sim), 
            output='screen', 
            emulate_tty=True
        ),

        Node(
            package='arm', 
            executable='resolver_spoofer', 
            namespace=namespace,
            output='screen', 
            condition=IfCondition(sim), 
            emulate_tty=True
        ),

        Node(
            package='arm', 
            executable='hex_key.py', 
            output='screen', 
            emulate_tty=True
        ),
        
        Node(
            package='arm',
            executable='lazers.py',
            output='screen',
            emulate_tty=True
        ),

        Node(
            package='gimbal_cam', 
            executable='gimbal_cam', 
            output='screen', 
            emulate_tty=True
        ),
    ])
