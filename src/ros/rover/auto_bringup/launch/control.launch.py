'''
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
'''
from launch import LaunchDescription
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch.conditions import UnlessCondition
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction, GroupAction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def launch_setup(context, *args, **kwargs):
    auto_bringup_dir = FindPackageShare('auto_bringup')
    
    controllers = LaunchConfiguration('controllers')
    gazebo = LaunchConfiguration('gazebo')
    model = LaunchConfiguration('model')

    return [
        Node(
            package='controller_manager',
            executable='spawner',
            arguments=['pivot_drive_controller', '--switch-timeout', '10'] #, '--inactive']
        ),
        GroupAction(
            condition=UnlessCondition(gazebo),
            actions=[
                Node(
                    package='controller_manager',
                    executable='spawner',
                    arguments=['joint_state_broadcaster']
                ),
                Node(
                    package='controller_manager',
                    executable='ros2_control_node',
                    parameters=[controllers],
                    remappings=[('/controller_manager/robot_description', '/robot_description')],
                ),
                IncludeLaunchDescription(
                    PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'urdf.launch.py'])),
                    launch_arguments={'model': model, 'gazebo': gazebo}.items(),
                )],
        ),
    ]

    model_arg = DeclareLaunchArgument(
        name="model",
        default_value=PathJoinSubstitution(
            [rover_description_dir, "urdf", "rover.urdf.xacro"]
        ),
        description="Absolute path to robot urdf file",
    )

    controllers_arg = DeclareLaunchArgument(
        name="controllers",
        default_value=PathJoinSubstitution(
            [auto_bringup_dir, "params", "controllers.yaml"]
        ),
        description="Path of the controller params file",
    )

def generate_launch_description():
    auto_bringup_dir = FindPackageShare('auto_bringup')
    rover_description_dir = FindPackageShare('rover_description')

    declared_arguments = [      
        DeclareLaunchArgument(
            name='controllers',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'controllers.yaml']),
            description='Absolute path to controller params file',
        ),
        DeclareLaunchArgument(
            name='gazebo',
            default_value='False',
            description='Use simulation (Gazebo) clock if True',
        ),
        DeclareLaunchArgument(
            name='model', 
            default_value=PathJoinSubstitution([rover_description_dir, 'base', 'urdf', 'rover.urdf.xacro']),
            description='Absolute path to robot urdf file',
        ),  
    ]

    return LaunchDescription(  
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )

    strafe_controller = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["strafe_controller"],
    )

    nova_diff_drive_controller = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["nova_diff_drive_controller", "--inactive"],
    )

    pivot_drive_controller = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["pivot_drive_controller", "--inactive"],
    )

    urdf_launch_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            PathJoinSubstitution([auto_bringup_dir, "launch", "urdf.launch.py"])
        ),
        condition=UnlessCondition(gazebo),
        launch_arguments={"model": model, "gazebo": "false"}.items(),
    )

    joint_broad = Node(
        package="controller_manager", executable="spawner", arguments=["joint_broad"]
    )

    return LaunchDescription(
        [
            gazebo_arg,
            model_arg,
            controllers_arg,
            urdf_launch_cmd,
            control_node,
            pivot_drive_controller,
            strafe_controller,
            # nova_diff_drive_controller,
            joint_broad,
        ]
    )
