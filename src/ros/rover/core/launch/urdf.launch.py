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

from ament_index_python.packages import get_package_share_path

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import (
    Command,
    LaunchConfiguration,
    FindExecutable,
    PathJoinSubstitution,
)
from launch.conditions import IfCondition, UnlessCondition

from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
import xacro

# Generate the launch file with all inputs
def generate_launch_description():
    gazebo = LaunchConfiguration('gazebo', default=False)
    core_path = get_package_share_path('core')
    default_model_path = core_path / 'urdf/rover.urdf.xacro'
    model_arg = DeclareLaunchArgument(name='model', default_value=str(default_model_path),
            description='Absolute path to robot urdf file')

    model = Command(
        [
            PathJoinSubstitution([FindExecutable(name='xacro')]),
            " ",
            default_model_path,
            " ",
            "gazebo:=",
            gazebo,
        ]
    )

    robot_description = ParameterValue(model, value_type=str)

    robot_state_publisher_node = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        parameters=[{'robot_description': robot_description}]
    )

    joint_state_publisher_node =  Node(
        package='control',
        executable='rover_state_publisher.py',
        output='screen',
        emulate_tty=True
    )

    return LaunchDescription([
        model_arg,
        robot_state_publisher_node,
        joint_state_publisher_node,
    ])