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
from launch_ros.substitutions import FindPackageShare


# Generate the launch file with all inputs
def generate_launch_description():
    core_dir = get_package_share_directory('core')
    gazebo = LaunchConfiguration('gazebo', default=False)
    model = LaunchConfiguration('model')
    controllers = LaunchConfiguration('controllers')

    gazebo_arg = DeclareLaunchArgument(
        'gazebo',
        default_value='true',
        description='Use simulation (Gazebo) clock if true')

    model_arg = DeclareLaunchArgument(name='model', default_value=PathJoinSubstitution([core_dir, 'urdf', 'rover.urdf.xacro']),
            description='Absolute path to robot urdf file')
    
    controllers_arg = DeclareLaunchArgument(
            name="controllers",
            default_value=PathJoinSubstitution(
                [
                    FindPackageShare("core"), 
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

    strafe_controller = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["strafe_controller", "--inactive"]
    )

    nova_diff_drive_controller = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["nova_diff_drive_controller", "--inactive"]
    )

    pivot_drive_controller = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["pivot_drive_controller"]
    )

    urdf_launch_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([core_dir, '/launch/urdf_launch.py']),
        condition=UnlessCondition(gazebo),
        launch_arguments={"model": model, "gazebo": 'false'}.items()
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
        pivot_drive_controller,
        strafe_controller,
        nova_diff_drive_controller,
        joint_broad,
    ])
