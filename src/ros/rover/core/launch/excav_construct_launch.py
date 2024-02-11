# Include the required launch parameters
from launch import LaunchDescription
import launch_ros.actions
from ament_index_python.packages import get_package_share_path

# Generate the launch file with all inputs
def generate_launch_description():
    # Get the path to the parameters file in core/params
    core_params_path = get_package_share_path('core') / "params"
    
    return LaunchDescription([      
        launch_ros.actions.Node(
            package='control', executable='scraper', output='screen', emulate_tty=True),
        launch_ros.actions.Node(
            package='control', executable='tile_placer', output='screen', emulate_tty=True),
    ])