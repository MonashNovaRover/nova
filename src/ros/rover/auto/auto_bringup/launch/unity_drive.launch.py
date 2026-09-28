from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess
from launch.substitutions import LaunchConfiguration
from launch.conditions import IfCondition
from launch_ros.actions import Node

def generate_launch_description():

    return LaunchDescription(
        [
            DeclareLaunchArgument(
                'start_sim',
                default_value='True'
            ),
            DeclareLaunchArgument(
                'world',
                default_value='ARC2025'
            ),
            DeclareLaunchArgument(
                'rover',
                default_value='default'
            ),

            ExecuteProcess(
                cmd=['ros2', 'launch', 'drive_bringup', 'drive.launch.py'],
                output='screen'
            ),
            Node(
                package='ros_tcp_endpoint',
                executable='default_server_endpoint',
            ),
            ExecuteProcess(
                cmd=['ros2', 'launch', 'teleop_drive_joy', 'teleop.launch.py'],
                output='screen'
            ),
            ExecuteProcess(
                cmd=[
                    'nova-unity-sim', 
                    '-screen-fullscreen', '0', 
                    '-window-mode', 'windowed', 
                    ['scene=', LaunchConfiguration("world")], 
                    ['robot=', LaunchConfiguration("rover")]],
                output='screen',
                condition=IfCondition(LaunchConfiguration("start_sim"))
            )
        ]
    )
