"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the rover to publish the urdf
    static transforms and associated joint states
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ NODES:
  - robot_state_publisher
  - rover_state_publisher
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	nova_bringup
CREATION:	13/05/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

from ament_index_python.packages import get_package_share_directory

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.substitutions import Command, LaunchConfiguration, PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare
from launch.conditions import IfCondition, UnlessCondition

from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue

def launch_setup(context, *args, **kwargs):
    if LaunchConfiguration('rover').perform(context) == "True":
        fixed_frame = 'base_link'
        robot_description = ParameterValue(
            Command(
                [
                    'xacro ', 
                    LaunchConfiguration('rover_urdf_path'),
                    ' arm:=',
                    LaunchConfiguration('arm'),
                    ' auto_camera:=false'
                ]
            ),
            value_type=str
        )
    else:
        fixed_frame = 'arm_link'
        robot_description = ParameterValue(
            Command(
                [
                    'xacro ', 
                    LaunchConfiguration('arm_urdf_path'),
                    ' auto_camera:=false'
                ]
            ),
            value_type=str
        )

    robot_state_publisher_node = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        parameters=[{'robot_description': robot_description}]
    )

    joint_state_publisher_node = Node(
        package='joint_state_publisher', 
        executable='joint_state_publisher', 
        namespace='',
        output='screen', 
        emulate_tty=True,
        parameters=[{
            'source_list': ['/arm/joint_states', '/joint_states']
        }]
    )

    rviz_node = Node(
        package='rviz2',
        namespace='',
        executable='rviz2',
        name='rviz2',
        arguments=['-d', [PathJoinSubstitution([FindPackageShare('nova_bringup'), 'rviz', 'default.rviz'])], '-f', fixed_frame]
    )

    return [
        robot_state_publisher_node,
        joint_state_publisher_node,
        rviz_node
    ]

# Generate the launch file with all inputs
def generate_launch_description():
    description_dir = FindPackageShare('rover_description')

    declared_arguments = [
        DeclareLaunchArgument(
            name='rover_urdf_path', 
            default_value=PathJoinSubstitution([description_dir, 'waratah', 'urdf', 'rover.urdf.xacro']),
            description='Absolute path to rover urdf file'
        ),
        DeclareLaunchArgument(
            name='arm_urdf_path', 
            default_value=PathJoinSubstitution([description_dir, 'arm', 'urdf', 'arm.urdf.xacro']),
            description='Absolute path to arm urdf file'
        ),
        DeclareLaunchArgument(
            name='arm', 
            default_value='True',
            description='Include arm URDF in robot_description'
        ),
        DeclareLaunchArgument(
            name='rover', 
            default_value='True',
            description='Include rover URDF in robot_description'
        ),
        DeclareLaunchArgument(
            name='rviz', 
            default_value='True',
            description='Launch rviz?'
        ),

    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
