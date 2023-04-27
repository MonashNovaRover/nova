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
from launch.substitutions import Command, LaunchConfiguration
from launch.conditions import IfCondition, UnlessCondition

from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue

# Generate the launch file with all inputs
def generate_launch_description():
    core_path = get_package_share_path('core')
    default_model_path = core_path / 'urdf/rover.urdf'

    from_tracking_cam = LaunchConfiguration('t265')

    tracking_cam_arg = DeclareLaunchArgument(
        name='t265',
        default_value='False',
        description="Set to 'True' to run localisation from the t265 tracking camera"
    )

    model_arg = DeclareLaunchArgument(name='model', default_value=str(default_model_path),
            description='Absolute path to robot urdf file')
    robot_description = ParameterValue(Command(['xacro ', LaunchConfiguration('model')]),
                                       value_type=str)

    world_to_map_node = Node(
            package='tf2_ros',
            executable='static_transform_publisher',
            arguments=['0', '0', '0', '0', '0', '0', 'world', 'map'],
            output='screen',
            emulate_tty=True
        )

    pose_converter_node = Node(
        package='autonomous',
        executable='pose_converter.py',
        output='screen',
        emulate_tty=True,
        condition=UnlessCondition(from_tracking_cam)
    )

    pose_converter_t265_node = Node(
        package='autonomous',
        executable='pose_converter_ARC.py',
        output='screen',
        emulate_tty=True,
        condition=IfCondition(from_tracking_cam)
    )

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
        tracking_cam_arg,
        model_arg,
        world_to_map_node,
        robot_state_publisher_node,
        pose_converter_node,
        pose_converter_t265_node,
        joint_state_publisher_node,
    ])
