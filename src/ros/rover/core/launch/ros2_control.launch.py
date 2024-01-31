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
from launch.actions import DeclareLaunchArgument
from launch.substitutions import (
    Command,
    LaunchConfiguration,
    FindExecutable,
    PathJoinSubstitution,
)
from launch.conditions import IfCondition, UnlessCondition

from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch_ros.substitutions import FindPackageShare
import xacro

# Generate the launch file with all inputs
def generate_launch_description():
    core_path = get_package_share_path('core')
    default_model_path = core_path / 'urdf/rover.urdf.xacro'
    
    #Declare Arguments
    declared_arguments = []
    #we aren't using the model argument here.. do we need this?
    declared_arguments.append(
        DeclareLaunchArgument(
            name='model', 
            default_value=str(default_model_path),
            description='Absolute path to robot urdf file'
        )
    )
    declared_arguments.append(
        DeclareLaunchArgument(
            name="use_mock_hardware",
            default_value="false",
            description="Start rover with mock hardware mirroring commands to its states"
        )
    )
    declared_arguments.append(
        DeclareLaunchArgument(
            name="gazebo",
            default_value="false",
            description="Run Gazebo simulation using gazebo_ros2_control"
        )
    )

    declared_arguments.append(
        DeclareLaunchArgument(
            name="urdf_path",
            default_value=PathJoinSubstitution(
                [
                    FindPackageShare("core"),
                    "urdf",
                    "rover.urdf.xacro"
                ]
            ),
            description="Path of the URDF file"
        )
    )

    declared_arguments.append(
        DeclareLaunchArgument(
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

    )

    model = LaunchConfiguration("model")
    use_mock_hardware = LaunchConfiguration("use_mock_hardware")
    gazebo = LaunchConfiguration("gazebo")
    urdf_path = LaunchConfiguration("urdf_path")
    controllers = LaunchConfiguration("controllers")
    

    robot_description_content = Command(
        [
            PathJoinSubstitution([FindExecutable(name='xacro')]),
            " ",
            urdf_path,
            " ",
            "gazebo:=",
            gazebo,
            " ",
            "use_mock_hardware:=",
            use_mock_hardware,
        ]
    )

    robot_description = {'robot_description': ParameterValue(robot_description_content, value_type=str)}


    control_node = Node(
        package="controller_manager",
        executable="ros2_control_node",
        parameters=[robot_description, controllers], # Deprecated: passing the robot description parameter directly to the control_manager node is deprecated. Use robot_state_publisher instead.
        arguments=['--ros-args'],# '--log-level','DEBUG'],
        #output="both" #added - not too sure what it does
    )

    wheel_velocity_controller = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["wheel_velocity_controller"]
    )

    pivot_position_controller = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["pivot_position_controller", "--inactive"]
    )

    joint_broad = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["joint_broad"]
    )

    pivot_drive_controller = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["pivot_drive_controller", "--inactive"]
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

    nodes = [
        control_node,
        wheel_velocity_controller,
        pivot_position_controller,
        joint_broad,
        pivot_drive_controller,
    ]

    return LaunchDescription(declared_arguments + nodes)
