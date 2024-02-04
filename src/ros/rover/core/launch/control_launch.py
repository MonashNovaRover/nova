"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the rover to start all
   rover control scripts.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - control/drive/drive_inputs      [drive_cmd]
  - control/drive/driver            [driver]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	core
CREATION:	15/12/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
from ament_index_python.packages import get_package_share_directory

# Include the required launch parameters
from launch import LaunchDescription
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch.conditions import UnlessCondition
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node


# Generate the launch file with all inputs
def generate_launch_description():
    core_dir = get_package_share_directory('core')
    gazebo = LaunchConfiguration('gazebo', default=False)
    model = LaunchConfiguration('model')

    gazebo_arg = DeclareLaunchArgument(
        'gazebo',
        default_value='true',
        description='Use simulation (Gazebo) clock if true')

    model_arg = DeclareLaunchArgument(name='model', default_value=PathJoinSubstitution([core_dir, 'urdf', 'rover.urdf.xacro']),
            description='Absolute path to robot urdf file')

    wheel_velocity_controller = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["wheel_velocity_controller"]
    )

    pivot_joint_trajectory_controller = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["pivot_joint_trajectory_controller"]
    )

    urdf_launch_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([core_dir, '/launch/urdf_launch.py']),
        condition=UnlessCondition(gazebo),
        launch_arguments={"model": model}.items()
    )

    joint_broad = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["joint_broad"]
    )

    inputs_processor = Node(
        package='control', 
        executable='drive_inputs', 
        emulate_tty=True,
        parameters=[{'use_sim_time': gazebo}]
    )
    
    driver = Node(
        package='control', 
        executable='driver', 
        output='screen', 
        emulate_tty=True,
        parameters=[{'use_sim_time': gazebo, 'gazebo': gazebo}]
    )

    led_publisher = Node(
        package='electronics', 
        executable='LED_transmitter.py', 
        output='screen', 
        emulate_tty=True,
        condition=UnlessCondition(gazebo)
    )

    return LaunchDescription([
        gazebo_arg,
        model_arg,
        wheel_velocity_controller,
        pivot_joint_trajectory_controller,
        urdf_launch_cmd,
        joint_broad,
        inputs_processor,
        driver,
        led_publisher
    ])
