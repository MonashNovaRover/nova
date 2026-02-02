import os
from launch import LaunchDescription
from launch.conditions.unless_condition import IfCondition
from launch.substitutions import PathJoinSubstitution, LaunchConfiguration
from launch.actions import DeclareLaunchArgument

from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from launch.actions import OpaqueFunction, ExecuteProcess

def launch_setup(context, *args, **kwargs):
    # package directories
    auto_bringup_dir = FindPackageShare('auto_bringup')

    gazebo = LaunchConfiguration('gazebo')
    rtabmap_viz = LaunchConfiguration('rtabmap_viz').perform(context)
    rviz_params = LaunchConfiguration('rviz_params')
    model = LaunchConfiguration('model').perform(context)


    rviz_node = Node(
        package='rviz2',
        namespace='',
        executable='rviz2',
        name='rviz2',
        arguments=['-d', [rviz_params]]
    )

    rtabmap_ros_node = Node(
        condition=IfCondition(rtabmap_viz),
        package='rtabmap_viz',
        executable='rtabmap_viz',
        output='screen',
        parameters=[{
            "subscribe_rgbd": True,
            "use_sim_time": gazebo}],
        remappings=[
            ('rgbd_image','oak/rgbd/image_raw'),
            ('odom', 'odom/visual'),
        ],
    )

    output_path = os.path.expanduser("~/rviz.urdf")
    local_urdf = ExecuteProcess(
        cmd=["xacro", model, '-o', output_path],
        output="screen"
    )

    return [rviz_node, rtabmap_ros_node, local_urdf]

def generate_launch_description():
    auto_bringup_dir = FindPackageShare('auto_bringup')
    rover_description_dir = FindPackageShare('rover_description')

    launch_args = [
        DeclareLaunchArgument(
            name='gazebo',
            default_value='false',
            description='',
        ),
        DeclareLaunchArgument(
            name='rtabmap_viz',
            default_value='false',
            description='Launch rtabmap_viz for mapping visualisation',
        ),
        DeclareLaunchArgument(
            name='rviz_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'rviz', 'navigation.rviz']),
            description='Full path to the RViz config file to use',
        ),
        DeclareLaunchArgument(
            name='model',
            default_value=PathJoinSubstitution([rover_description_dir, 'banksia', 'urdf', 'rover.urdf.xacro']),
            description='Absolute path to robot urdf file',
        ),
    ]

    return LaunchDescription( launch_args + [OpaqueFunction(function=launch_setup)])
