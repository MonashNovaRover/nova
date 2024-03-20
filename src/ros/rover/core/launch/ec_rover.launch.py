# Include the required launch parameters
import os
from ament_index_python import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch_ros.actions import Node
from launch.launch_description_sources import PythonLaunchDescriptionSource

# Generate the launch file with all inputs
def generate_launch_description():
    core_dir = get_package_share_directory('core')
    
    return LaunchDescription([      
        Node(
            package='control', executable='scraper.py', output='screen', emulate_tty=True),
        Node(
            package='control', executable='tile_placer.py', output='screen', emulate_tty=True),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(os.path.join(core_dir, 'launch', 'drive.launch.py'))),
    ])