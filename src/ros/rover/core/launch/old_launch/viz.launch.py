from ament_index_python.packages import get_package_share_path

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess
from launch.substitutions import LaunchConfiguration

from launch_ros.actions import Node

def generate_launch_description():
    core_path = get_package_share_path('core')
    default_rviz_path = (core_path / 'rviz' / 'rover.rviz').resolve()
    assert default_rviz_path.is_file()

    rviz_node = ExecuteProcess(
        cmd=[
            'rviz2',
            '--display-config', str(default_rviz_path)
        ],
        output='screen'
    )

    return LaunchDescription([
        rviz_node,
    ])
