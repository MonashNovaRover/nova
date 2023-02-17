from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch_ros.substitutions import FindPackageShare
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node

# For launching rviz
import os
from ament_index_python.packages import get_package_share_directory
# directory of rviz config file
auto_path = os.path.realpath(get_package_share_directory('autonomous'))
rviz_path = os.path.join(auto_path, 'rviz', 'auto.rviz')


def generate_launch_description():
    return LaunchDescription([
        # rviz
        Node(
            package='rviz2',
            node_executable='rviz2',
            name="rviz2",
            output='screen',
            arguments=['-d', str(rviz_path)],
            emulate_tty=True
        ),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource([
                FindPackageShare("control"), '/launch', '/rover_visualisation.launch.py'
            ])
        ),
        # autonomous nodes
        Node(
            package="autonomous",
            node_executable="path_vis.py",
            output="screen",
            emulate_tty=True
        ),
        Node(
            package='autonomous',
            node_executable='rover_vis.py',
            output='screen',
            emulate_tty=True
        ),
        Node(
            package="autonomous",
            node_executable="update_goals.py",
            output="screen",
            emulate_tty=True
        ),
    ])
