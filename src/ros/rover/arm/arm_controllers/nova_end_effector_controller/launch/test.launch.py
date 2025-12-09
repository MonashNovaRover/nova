
# Include the required launch parameters
from launch import LaunchDescription
from launch.substitutions import LaunchConfiguration
from launch.conditions import UnlessCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction, GroupAction
from launch_ros.actions import Node


# Generate the launch file with all inputs
def generate_launch_description():
    gazebo = False
    model = 'rover7'
    return LaunchDescription([
        Node(
            package='controller_manager',
            executable='spawner',
            arguments=['velocity_controller'], #, '-t', 'velocity_controller/nova_end_effector_controller'],
        ),
        GroupAction(
            actions=[
                Node(
                    package='controller_manager',
                    executable='spawner',
                    arguments=['joint_state_broadcaster']
                ),
                Node(
                    package='controller_manager',
                    executable='ros2_control_node',
                    parameters=['nova_end_effector_controller'],
                    remappings=[('/controller_manager/robot_description', '/robot_description')],
                ),
                ],
        ),
    ])
