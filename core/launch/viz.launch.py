from ament_index_python.packages import get_package_share_path

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition, UnlessCondition
from launch.substitutions import Command, LaunchConfiguration

from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue

def generate_launch_description():
    core_path = get_package_share_path('core')
    default_rviz_path = core_path / 'rviz/rover.rviz'
    default_model_path = core_path / 'urdf/rover.urdf'

    model_arg = DeclareLaunchArgument(name='model', default_value=str(default_model_path),
            description='Absolute path to robot urdf file')
    rviz_arg = DeclareLaunchArgument(name='rvizconfig', default_value=str(default_rviz_path),
            description='Absolute path to rviz config file')

    robot_description = ParameterValue(Command(['xacro ', LaunchConfiguration('model')]),
                                       value_type=str)

    robot_state_publisher_node = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        parameters=[{'robot_description': robot_description}]
    )

    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        output='screen',
        arguments=['-d', LaunchConfiguration('rvizconfig')],
    )

    joint_state_publisher_node =  Node(
        package='control',
        executable='rover_state_publisher.py',
        output='screen',
        emulate_tty=True
    )

    return LaunchDescription([
        model_arg,
        rviz_arg,
        rviz_node,
        joint_state_publisher_node,
        robot_state_publisher_node,
    ])
