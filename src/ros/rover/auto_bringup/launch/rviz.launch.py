from launch import LaunchDescription
from launch.conditions.unless_condition import IfCondition
from launch.substitutions import PathJoinSubstitution, LaunchConfiguration
from launch.actions import DeclareLaunchArgument

from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from launch.actions import OpaqueFunction

def launch_setup(context, *args, **kwargs):
    gazebo = LaunchConfiguration('gazebo')
    rtabmap_viz = LaunchConfiguration('rtabmap_viz').perform(context)
    rviz_params = LaunchConfiguration('rviz_params')


    rviz_node = Node(
        package='rviz2',
        namespace='',
        executable='rviz2',
        name='rviz2',
        arguments=['-d', [PathJoinSubstitution([FindPackageShare('auto_bringup'), 'rviz', rviz_params])]]
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

    return [rviz_node, rtabmap_ros_node]

def generate_launch_description():

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
            default_value='navigation.rviz',
            description='',
        ),
    ]

    return LaunchDescription( launch_args + [OpaqueFunction(function=launch_setup)])
