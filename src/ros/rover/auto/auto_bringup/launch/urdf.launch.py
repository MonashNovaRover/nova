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
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.conditions import IfCondition
from launch.substitutions import Command, LaunchConfiguration, PathJoinSubstitution

from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    oak_angle = LaunchConfiguration('oak_angle').perform(context)
    bootie_angle = LaunchConfiguration('bootie_angle').perform(context)
    lidar_angle = LaunchConfiguration('lidar_angle').perform(context)
    gazebo = LaunchConfiguration('gazebo').perform(context)
    model = LaunchConfiguration('model').perform(context)
    robot_name = LaunchConfiguration('robot_name').perform(context)
    camera = LaunchConfiguration('camera').perform(context)
    joints = LaunchConfiguration('joints').perform(context)

    return [
        Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            parameters=[{'robot_description': 
                ParameterValue(Command(['xacro ', 
                                        model, ' ', 
                                        'gazebo:=', gazebo, ' ', 
                                        'robot_name:=', robot_name, ' ', 
                                        'lidar_angle:=', lidar_angle, ' ', 
                                        'oak_angle:=', oak_angle, ' ',
                                        'bootie_angle:=', bootie_angle, ' ', 
                                        'auto_camera:=', camera
                                       ]), value_type=str)
            }]
        ),
        Node(
            condition=IfCondition(joints),
            package = 'joint_state_publisher',
            executable = 'joint_state_publisher',
            parameters=[{'source_list': ['/joint_states']}],
        ),
    ]


def generate_launch_description():
    rover_description_dir = FindPackageShare('rover_description')

    declared_arguments = [
        DeclareLaunchArgument(
            name='oak_angle',
            default_value='15',
            description='Angle (in degrees) at which the oak front camera is mounted',
        ),
        DeclareLaunchArgument(
            name='bootie_angle',
            default_value='15',
            description='Angle (in degrees) at which the bootie back camera is mounted',
        ),
        DeclareLaunchArgument(
            name='lidar_angle',
            default_value='40',
            description='Angle (in degrees) at which the lidar is mounted',
        ),
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
            name='robot_name',
            default_value='Banksia',
            description='name of the robot',
        ),
        DeclareLaunchArgument(
            name='camera',
            default_value='True',
            description='Whether to spawn auto mount on the rover.',
        ),
        DeclareLaunchArgument(
            name='joints',
            default_value='False',
            description='Whether to launch joint_state_publisher.',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
