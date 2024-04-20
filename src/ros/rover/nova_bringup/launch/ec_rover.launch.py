# Include the required launch parameters
import os
from ament_index_python import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, DeclareLaunchArgument
from launch_ros.actions import Node
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare


# Generate the launch file with all inputs
def generate_launch_description():
    nova_bringup_dir = FindPackageShare('nova_bringup')
    
    return LaunchDescription([ 
        DeclareLaunchArgument('scraper_card_type', default_value='CMD', description='Card type for the scraper node'),     
        Node(
            package='excavation_construction', executable='scraper', output='screen', emulate_tty=True, parameters=[{'card_type': LaunchConfiguration('scraper_card_type')}]),
        Node(
            package='excavation_construction', executable='tile_placer', output='screen', emulate_tty=True),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(PathJoinSubstitution([nova_bringup_dir, 'launch', 'drive.launch.py']))),
    ])