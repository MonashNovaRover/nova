from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction, GroupAction, TimerAction, ExecuteProcess, RegisterEventHandler
from launch.substitutions import  PathJoinSubstitution, LaunchConfiguration
from launch.conditions import IfCondition, UnlessCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from launch.event_handlers import OnProcessExit

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
                cmd=['ros2', 'launch', 'drive_bringup', 'drive.launch.py', 'sim:=True'],
                output='screen'
            ),
            Node(
                package='ros_tcp_endpoint',
                executable='default_server_endpoint',
            ),
            ExecuteProcess(
                cmd=['ros2', 'launch', 'auto_bringup', 'urdf.launch.py', 'sim:=True'],
                output='screen'
            ),
            ExecuteProcess(
                cmd=['ros2', 'launch', 'teleop_drive_joy', 'teleop.launch.py'],
                output='screen'
            ),
            ExecuteProcess(
                cmd=['nova-unity-sim', '-screen-fullscreen', '0', ['scene=', LaunchConfiguration("world")], ['robot=', LaunchConfiguration("rover")]],
                output='screen',
                condition=IfCondition(LaunchConfiguration("start_sim"))
            )
        ]
    )
