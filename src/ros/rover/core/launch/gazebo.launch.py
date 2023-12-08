"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the rover to publish the urdf
    static transforms and associated joint states
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - robot_state_publisher
  - rover_state_publisher
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	core
CREATION:	27/04/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

from ament_index_python.packages import get_package_share_path

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.substitutions import Command, FindExecutable, PathJoinSubstitution, LaunchConfiguration
from launch.conditions import IfCondition, UnlessCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource

from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch_ros.substitutions import FindPackageShare

# Generate the launch file with all inputs
def generate_launch_description():
    core_path = get_package_share_path('core')
    default_model_path = core_path / 'urdf/rover.urdf.xacro'

    model_arg = DeclareLaunchArgument(name='model', default_value=str(default_model_path),
            description='Absolute path to robot urdf file')
    robot_description = ParameterValue(
        Command(
            [
                'xacro ', 
                LaunchConfiguration('model'),
                " ",
                "gazebo:=",
                "true"
            ]
        ),
        value_type=str
    )

    robot_state_publisher_node = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        parameters=[{'robot_description': robot_description}]
    )

    gazebo = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            [PathJoinSubstitution([FindPackageShare("gazebo_ros"), "launch", "gazebo.launch.py"])]
        ),
        launch_arguments={"verbose": "true"}.items(),
    )

    spawn_entity = Node(
        package='gazebo_ros',
        executable='spawn_entity.py',
        arguments=["-topic", "robot_description", "-entity", "Waratah"]
    )

    
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
    '''

    four_wheel_steering_controller = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["four_wheel_steering_controller"]
    )

    four_steering_controller = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["four_steering_controller"]
    )

    ''' 
    pivot_drive_controller = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["pivot_drive_controller"]
    )

    diff_drive_controller = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["diff_drive_controller"]
    )


    joint_broad = Node(
        package="controller_manager",
        executable="spawner.py",
        arguments=["joint_broad"],
    )

    return LaunchDescription([
        model_arg,
        robot_state_publisher_node,
        gazebo,
        spawn_entity,
        #wheel_velocity_controller,
        #pivot_joint_trajectory_controller,
        #four_wheel_steering_controller,
        #four_steering_controller,
        #diff_drive_controller,
        pivot_drive_controller,
    ])
