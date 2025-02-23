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
from launch.substitutions import Command, LaunchConfiguration, PathJoinSubstitution

from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    angle = LaunchConfiguration('angle').perform(context)
    gazebo = LaunchConfiguration('gazebo').perform(context)
    model = LaunchConfiguration('model').perform(context)
    robot_name = LaunchConfiguration('robot_name').perform(context)
    arm = LaunchConfiguration('arm').perform(context)
    use_mock_hardware = LaunchConfiguration('use_mock_hardware').perform(context)

    return [
        Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            parameters=[{'robot_description': 
                ParameterValue(Command(['xacro ', model, ' ', 'gazebo:=', gazebo, ' ', 'robot_name:=', robot_name, ' ', 'angle:=', angle, ' ', 'arm:=', arm, ' ', 'use_mock_hardware:=', use_mock_hardware]), value_type=str)
            }]
        ),
        # Launch joint states for arm
        Node(
            package='joint_state_publisher',
            executable='joint_state_publisher',
            namespace='',
            output='screen',
            emulate_tty=True,
            parameters=[{
                'source_list': ['/arm/joint_states', '/joint_states']
            }]
        )
    ]


def generate_launch_description():
    rover_description_dir = FindPackageShare('rover_description')

    declared_arguments = [
        DeclareLaunchArgument(
            name='angle',
            default_value='15',
            description='Angle (in degrees) at which the camera is mounted',
        ),
        DeclareLaunchArgument(
            name='gazebo', 
            default_value='True',
            description='Launch with gazebo or not',
        ),
        DeclareLaunchArgument(
            name='model',
            default_value=PathJoinSubstitution([rover_description_dir, 'rover7', 'urdf', 'rover.urdf.xacro']),
            description='Absolute path to robot urdf file',
        ),
        DeclareLaunchArgument(
            name='robot_name',
            default_value='Rover7',
            description='name of the robot',
        ),
        DeclareLaunchArgument(
            name='arm',
            default_value='false',
            description='whether to launch arm',
        ),
        DeclareLaunchArgument(
            name='use_mock_hardware',
            default_value='false',
            description='whether to use mock hardware for hardware interfaces',
        )
    ]
    
    return LaunchDescription(  
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )