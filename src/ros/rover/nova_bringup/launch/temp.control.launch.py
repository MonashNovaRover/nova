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
PACKAGE:    nova_bringup
CREATION:	13/12/2024
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
from launch_ros.substitutions import FindPackageShare
from launch_ros.descriptions import ParameterValue


# Generate the launch file with all inputs
def generate_launch_description():
    nova_bringup_dir = FindPackageShare('nova_bringup')
    rover_description_dir = FindPackageShare('rover_description')
    gazebo = LaunchConfiguration('gazebo', default=False)
    model = LaunchConfiguration('model')
    controllers = LaunchConfiguration('controllers')

    gazebo_arg = DeclareLaunchArgument(
        'gazebo',
        default_value='False',
        description='Use simulation (Gazebo) clock if true')

    model_arg = DeclareLaunchArgument(name='model', 
            default_value=PathJoinSubstitution([rover_description_dir, 'urdf', 'generic_can_ros2_control.xacro']),
            description='Absolute path to robot urdf file')
    
    controllers_arg = DeclareLaunchArgument(
            name="controllers",
            default_value=PathJoinSubstitution(
                [
                    nova_bringup_dir,
                    "params", 
                    "controllers.yaml"
                ]       
            ),
            description="Path of the controller params file"
        )

    control_node = Node(
        package="controller_manager",
        executable="ros2_control_node",
        parameters=[controllers],
        remappings=[('/controller_manager/robot_description', '/robot_description')],
        condition = UnlessCondition(gazebo)
    )

    generic_broadcaster = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["generic_broadcaster"]
    )

    urdf_launch_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(PathJoinSubstitution([nova_bringup_dir, 'launch', 'urdf.launch.py'])),
        condition=UnlessCondition(gazebo),
        launch_arguments={"model": PathJoinSubstitution([rover_description_dir, 'urdf', 'generic_can_ros2_control.xacro']), "gazebo": 'false'}.items()
    )

    joint_broad = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["joint_broad"]
    )

    return LaunchDescription([
        gazebo_arg,
        model_arg,
        controllers_arg,
        urdf_launch_cmd,
        control_node,
        generic_broadcaster,
        joint_broad,
    ])
