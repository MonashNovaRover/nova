"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the rover to start all
   science scripts.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - science/science_transmitter.py      [science_transmitter]
  - science/distance_publisher.py       [distance_data]
  - science/spectrometer_publisher.py   [spectrometer_data]
  - science/EMC_publisher.py            [emc_data]
  - science/kiln_data_publisher.py      [kiln_mass_data, kiln_temp_data]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	core
CREATION:	17/12/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

# Include the required launch parameters
from launch import LaunchDescription
import launch_ros.actions

# Generate the launch file with all inputs
def generate_launch_description():
    return LaunchDescription([
        launch_ros.actions.Node(
            package='science', executable='transmitter.py', output='screen', emulate_tty=True),
        # launch_ros.actions.Node(
        #     package='science', executable='distance_publisher.py', output='screen', emulate_tty=True),
        launch_ros.actions.Node(
            package='science', executable='spectrometer_publisher.py', output='screen', emulate_tty=True),
        launch_ros.actions.Node(
            package='science', executable='EMC_publisher.py', output='screen', emulate_tty=True),
        #launch_ros.actions.Node(
        #    package='science', executable='kilns_data_publisher.py', output='screen', emulate_tty=True),
        #launch_ros.actions.Node(
        #    package='science', executable='actuator_limit_publisher.py', output='screen', emulate_tty=True),
    ])
