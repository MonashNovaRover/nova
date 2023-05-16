from ament_index_python.packages import get_package_share_path

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration

from launch_ros.actions import Node

def generate_launch_description():
    core_path = get_package_share_path('core')
    default_rviz_path = core_path / 'rviz/rover.rviz'

    rviz_arg = DeclareLaunchArgument(name='rvizconfig', default_value=str(default_rviz_path),
            description='Absolute path to rviz config file')

    marker_vis_node = Node(
        package='autonomous',
        executable='marker_vis.py',
        output='screen',
        emulate_tty=True
    )

    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        output='screen',
        arguments=['-d', LaunchConfiguration('rvizconfig')],
    )

    return LaunchDescription([
        rviz_arg,
        rviz_node,
        marker_vis_node
    ])
