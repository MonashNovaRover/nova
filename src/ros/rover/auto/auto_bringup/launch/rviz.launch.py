from launch import LaunchDescription
from launch.substitutions import PathJoinSubstitution, LaunchConfiguration, IfElseSubstitution
from launch.actions import DeclareLaunchArgument

from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from launch.actions import OpaqueFunction, ExecuteProcess

from os.path import expanduser, exists

def launch_setup(context, *args, **kwargs):
    # package directories
    local = LaunchConfiguration('local')

    auto_bringup_dir = IfElseSubstitution(local,
        PathJoinSubstitution([expanduser('~'), '/nova/src/ros/rover/auto/auto_bringup']),
        FindPackageShare('auto_bringup')
    )

    gazebo = LaunchConfiguration('gazebo').perform(context)
    rviz_params = LaunchConfiguration('rviz_params').perform(context)
    model = LaunchConfiguration('model').perform(context)
    shortened_auto_mount = LaunchConfiguration('shortened_auto_mount').perform(context)
    robot_name = LaunchConfiguration('robot_name').perform(context)
    
    rviz_params = PathJoinSubstitution([auto_bringup_dir, 'rviz', rviz_params + '.rviz'])
    if not exists(rviz_params.perform(context)):
        raise ValueError(f"RViz config file {rviz_params.perform(context)} does not exist")

    return [
        Node(
            package='rviz2',
            namespace='',
            executable='rviz2',
            name='rviz2',
            arguments=['-d', [rviz_params]]
        ),
        ExecuteProcess(
            cmd=['xacro', model,
                 f'gazebo:={gazebo}',
                 f'robot_name:={robot_name}',
                 'auto_mount:=True',
                 f'shortened_auto_mount:={shortened_auto_mount}',
                 '-o', expanduser("~/rviz.urdf")],
            output="screen"
        ),
    ]

def generate_launch_description():
    local = LaunchConfiguration('local')

    rover_description_dir = IfElseSubstitution(local,
        PathJoinSubstitution([expanduser('~'), '/nova/src/ros/rover/rover_description']),
        FindPackageShare('rover_description')
    )

    declared_arguments = [
        DeclareLaunchArgument(
            name='local',
            default_value='False',
            description='Whether to use local directories instead of the nix store.',
        ),
        DeclareLaunchArgument(
            name='gazebo',
            default_value='false',
            description='',
        ),
        DeclareLaunchArgument(
            name='rviz_params',
            default_value='everything',
            description='Name of the rviz config file to use, without the .rviz extension. Must be located in src/ros/rover/auto/auto_bringup/rviz',
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
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
