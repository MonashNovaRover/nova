'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code in the base station to get 
    oak cameras to GUI.
Run camera.launch.py before running this launch!
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - image_republishers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	auto_bringup
CREATION:	13/11/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction
from launch.conditions import IfCondition, UnlessCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, AndSubstitution, NotSubstitution
from launch_ros.actions import  Node, ComposableNodeContainer
from launch_ros.descriptions import ComposableNode
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    auto_bringup_dir = FindPackageShare('auto_bringup')
    cameras2_dir = FindPackageShare("cameras2")
    back = LaunchConfiguration('back')
    front = LaunchConfiguration('front')
    signalling = LaunchConfiguration('signalling') # set to true is cameras2 is not running alongside

    return [
        Node(
            condition=IfCondition(front),
            package='image_transport',
            executable='republish',
            parameters=[{
                'in_transport':'compressed', 
                'out_transport':'raw'}],
            remappings=[
                ('/in/compressed','/oak/rgb/image_raw/compressed'),
                ('/out','/oak/rgb/gui')], # ros camera topic for gui
        ),
        Node(
            condition=IfCondition(front),
            package='image_transport',
            executable='republish',
            parameters=[{
                'in_transport':'compressed', 
                'out_transport':   'raw'}],
            remappings=[
                 ('/in/compressed','/bootie/rgb/image_raw/compressed'),
                 ('/out','/bootie/rgb/gui')], # ros camera topic for gui
        ),
        IncludeLaunchDescription(
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([cameras2_dir, 'ros_topic_streamer.launch.py'])),
            launch_arguments={'signalling': signalling}.items(),
        ),
    ]


def generate_launch_description():
    auto_bringup_dir = FindPackageShare('auto_bringup')

    declared_arguments = [
        DeclareLaunchArgument(
            name='back',
            default_value='True',
            description='',
        ),
        DeclareLaunchArgument(
            name='front',
            default_value='True',
            description='',
        ),
        DeclareLaunchArgument(
            name='signalling',
            default_value='False',
            description='',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )