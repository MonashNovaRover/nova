
# Include the required launch parameters
from launch import LaunchDescription
from launch.substitutions import LaunchConfiguration
from launch.conditions import UnlessCondition
from launch_ros.actions import Node


# Generate the launch file with all inputs
def generate_launch_description():
    return LaunchDescription([
        Node(
            package='nova_arm_controller', executable='nova_arm_controller', output='screen',
        ),
    ]
                             )
