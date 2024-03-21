# Include the required launch parameters
import os
from ament_index_python import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, DeclareLaunchArgument
from launch_ros.actions import Node
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration


# Generate the launch file with all inputs
def generate_launch_description():
    core_dir = get_package_share_directory('core')
    
    return LaunchDescription([ 
        DeclareLaunchArgument('scraper_card_type', default_value='CMD', description='Card type for the scraper node'),     
        Node(
            package='control', executable='scraper.py', output='screen', emulate_tty=True, parameters=[{'card_type': LaunchConfiguration('scraper_card_type')}]),
        Node(
            package='control', executable='tile_placer.py', output='screen', emulate_tty=True),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(os.path.join(core_dir, 'launch', 'drive.launch.py'))),
    ])