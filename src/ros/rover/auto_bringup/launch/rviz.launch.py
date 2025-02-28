from launch import LaunchDescription
from launch.conditions.unless_condition import IfCondition
from launch.substitutions import PathJoinSubstitution, LaunchConfiguration
from launch.actions import DeclareLaunchArgument

from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from launch.actions import OpaqueFunction

def launch_setup(context, *args, **kwargs):
    rtabmap_viz = LaunchConfiguration('rtabmap_viz').perform(context)
    use_sim_time = LaunchConfiguration('use_sim_time')
    config = LaunchConfiguration('config')


    rviz_node = Node(
        package='rviz2',
        namespace='',
        executable='rviz2',
        name='rviz2',
        arguments=['-d', [PathJoinSubstitution([FindPackageShare('auto_bringup'), 'rviz', config])]]
    )

    rtabmap_ros_node = Node(
        condition=IfCondition(rtabmap_viz),
        package='rtabmap_viz',
        executable='rtabmap_viz',
        output='screen',
        parameters=[{"subscribe_rgbd":True,"use_sim_time":use_sim_time}],
        remappings=[
            ('rgbd_image','oak/rgbd/image_raw'),
            ('odom', 'odom/visual'),
        ],
    )

    return [rviz_node, rtabmap_ros_node]

def generate_launch_description():

    launch_args = [
        DeclareLaunchArgument(
            'rtabmap_viz', default_value='false',
            description='Launch rtabmap_viz for mapping visualisation'),

        DeclareLaunchArgument('use_sim_time', default_value='false'),
        DeclareLaunchArgument('config', default_value='navigation.rviz'),
    ]

    return LaunchDescription( launch_args + [OpaqueFunction(function=launch_setup)])
