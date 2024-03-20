# Include the required launch parameters
from launch import LaunchDescription
from launch_ros.actions import Node

# Generate the launch file with all inputs
def generate_launch_description():
    
    return LaunchDescription([      
        Node(
            package='control', executable='scraper.py', output='screen', emulate_tty=True),
        Node(
            package='control', executable='tile_placer.py', output='screen', emulate_tty=True),
    ])