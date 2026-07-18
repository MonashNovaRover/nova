'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the rover to publish the urdf
    static transforms and associated joint states
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
INCLUDED LAUNCH FILES:
- rviz.launch.py

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
from launch.substitutions import Command, LaunchConfiguration, PathJoinSubstitution, IfElseSubstitution

from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.substitutions import FindPackageShare

from os.path import expanduser, exists

def launch_setup(context, *args, **kwargs):
    # package directories
    local = LaunchConfiguration('local')

    auto_bringup_dir = IfElseSubstitution(local,
        PathJoinSubstitution([expanduser("~") + '/nova/src/ros/rover/auto/auto_bringup']),
        FindPackageShare('auto_bringup')
    )

    sim = LaunchConfiguration('sim')
    urdf_path = LaunchConfiguration('urdf_path').perform(context)
    robot_name = LaunchConfiguration('robot_name').perform(context)
    rviz = LaunchConfiguration('rviz')
    rviz_params = LaunchConfiguration('rviz_params')

    robot_description = ParameterValue(Command([
        'xacro ', 
        urdf_path, ' ',
        'sim:=', sim.perform(context), ' ',
        'robot_name:=', robot_name, ' ',
        'auto_mount:=', 'true']), 
    value_type=str)
    
    return [
        Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            parameters=[{'use_sim_time': sim},
                        {'robot_description': robot_description}],
            remappings=[('joint_states', 'drive/joint_states')],
        ),
        IncludeLaunchDescription(
            condition=IfCondition(rviz),
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'rviz.launch.py'])),
            launch_arguments={'sim': sim, 'urdf_path': urdf_path, 'robot_name': robot_name, 'rviz_params': rviz_params}.items(),
        ),
    ]


def generate_launch_description():
    local = LaunchConfiguration('local')

    rover_description_dir = IfElseSubstitution(local,
        PathJoinSubstitution([expanduser("~") + '/nova/src/ros/rover/rover_description']),
        FindPackageShare('rover_description')
    )

    declared_arguments = [
        DeclareLaunchArgument(
            name='local',
            default_value='False',
            description='Whether to use local directories instead of the nix store.',
        ),
        DeclareLaunchArgument(
            name='sim', 
            default_value='False',
            description='Use simulation clock if True',
        ),
        DeclareLaunchArgument(
            name='urdf_path',
            default_value=PathJoinSubstitution([rover_description_dir, 'banksia', 'urdf', 'rover.urdf.xacro']),
            description='Absolute path to robot urdf file',
        ),
        DeclareLaunchArgument(
            name='robot_name',
            default_value='Banksia',
            description='name of the robot',
        ),
        DeclareLaunchArgument(
            name='rviz',
            default_value='False',
            description='Whether to launch RViz2.',
        ),
        DeclareLaunchArgument( # Do not include 'rviz' argument in nested launch files https://github.com/ros2/launch/issues/313
            name='rviz_params',
            default_value='everything',
            description='Name of the rviz config file to use, without the .rviz extension. Must be located in src/ros/rover/auto/auto_bringup/rviz',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
