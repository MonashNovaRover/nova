'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the rover to publish the urdf
    static transforms and associated joint states
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - robot_state_publisher
  - rover_state_publisher
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	auto_bringup
CREATION:	27/04/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.substitutions import Command, LaunchConfiguration, PathJoinSubstitution

from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    auto_bringup_dir = FindPackageShare('auto_bringup')

    gazebo = LaunchConfiguration('gazebo').perform(context)
    model = LaunchConfiguration('model').perform(context)
    shortened_auto_mount = LaunchConfiguration('shortened_auto_mount').perform(context)
    robot_name = LaunchConfiguration('robot_name').perform(context)
    joints = LaunchConfiguration('joints').perform(context)
    rviz = LaunchConfiguration('rviz')
    rviz_params = LaunchConfiguration('rviz_params')

    return [
        Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            parameters=[{'robot_description': 
                ParameterValue(Command(['xacro ', 
                                        model, ' ', 
                                        'gazebo:=', gazebo, ' ', 
                                        'robot_name:=', robot_name, ' ',
                                        'auto_mount:=', 'true', ' ',
                                        'shortened_auto_mount:=', shortened_auto_mount
                                       ]), value_type=str)
            }]
        ),
        Node(
            condition=IfCondition(joints),
            package = 'joint_state_publisher',
            executable = 'joint_state_publisher',
            parameters=[{'source_list': ['/joint_states']}],
            output='screen',
        ),
        IncludeLaunchDescription(
            condition=IfCondition(rviz),
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'rviz.launch.py'])),
            launch_arguments={'gazebo': gazebo, 'model': model, 'shortened_auto_mount': shortened_auto_mount, 'robot_name': robot_name, 'rviz_params': rviz_params}.items(),
        ),
    ]


def generate_launch_description():
    rover_description_dir = FindPackageShare('rover_description')
    auto_bringup_dir = FindPackageShare('auto_bringup')

    declared_arguments = [
        DeclareLaunchArgument(
            name='gazebo', 
            default_value='False',
            description='Launch with gazebo or not',
        ),
        DeclareLaunchArgument(
            name='model',
            default_value=PathJoinSubstitution([rover_description_dir, 'banksia', 'urdf', 'rover.urdf.xacro']),
            description='Absolute path to robot urdf file',
        ),
        DeclareLaunchArgument(
            name='shortened_auto_mount',
            default_value='True',
            description='Whether to use the shortened auto mount model',
        ),
        DeclareLaunchArgument(
            name='robot_name',
            default_value='Banksia',
            description='name of the robot',
        ),
        DeclareLaunchArgument(
            name='joints',
            default_value='False',
            description='Whether to launch joint_state_publisher.',
        ),
        DeclareLaunchArgument(
            name='rviz',
            default_value='False',
            description='Whether to launch RViz2.',
        ),
        DeclareLaunchArgument( # Do not include 'rviz' argument in nested launch files https://github.com/ros2/launch/issues/313
            name='rviz_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'rviz', 'everything.rviz']),
            description='Full path to the RViz config file to use',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
