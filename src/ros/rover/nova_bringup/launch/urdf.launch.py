"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the rover to publish the urdf
    static transforms and associated joint states
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - robot_state_publisher
  - rover_state_publisher
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	core
CREATION:	27/04/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

from ament_index_python.packages import get_package_share_directory

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import Command, LaunchConfiguration, PathJoinSubstitution
from launch.conditions import IfCondition, UnlessCondition

from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue

# Generate the launch file with all inputs
def generate_launch_description():
    description_dir = get_package_share_directory('rover_description')

    gazebo = LaunchConfiguration('gazebo')

    model_arg = DeclareLaunchArgument(name='model', default_value=PathJoinSubstitution([description_dir, 'urdf', 'arm_rover.urdf']),
            description='Absolute path to robot urdf file')

    gazebo_arg = DeclareLaunchArgument(
        'gazebo', 
        default_value='true',
        description='Launch with gazebo or not')

    robot_description = ParameterValue(
        Command(
            [
                'urdf ', 
                LaunchConfiguration('model'),
                " ",
                "gazebo:=",
                gazebo
            ]
        ),
        value_type=str
    )

    robot_state_publisher_node = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        parameters=[{'robot_description': robot_description}]
    )

    return LaunchDescription([
        model_arg,
        gazebo_arg,
        robot_state_publisher_node,
    ])