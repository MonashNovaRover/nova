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
PACKAGE: 	arm_bringup
CREATION:	27/04/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.substitutions import Command, LaunchConfiguration, PathJoinSubstitution

from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch_ros.substitutions import FindPackageShare
from launch.conditions import IfCondition

def launch_setup(context, *args, **kwargs):
    arm_bringup_dir = FindPackageShare('arm_bringup')

    gazebo = LaunchConfiguration('gazebo').perform(context)
    model = LaunchConfiguration('model').perform(context)
    robot_name = LaunchConfiguration('robot_name').perform(context)
    arm = LaunchConfiguration('arm').perform(context)
    old_arm = LaunchConfiguration('old_arm').perform(context)
    use_mock_hardware = LaunchConfiguration('use_mock_hardware').perform(context)
    rviz = LaunchConfiguration('rviz')

    fixed_frame = 'base_link'

    xacro_args = ['gazebo:=', gazebo, ' ', 'robot_name:=', robot_name, ' ', 'arm:=', arm, ' ', 'old_arm:=', old_arm, ' ', 'use_mock_hardware:=', use_mock_hardware, ' ', 'auto_camera:=false']
    if LaunchConfiguration('local').perform(context):
        xacro_args.append(' rover_description_dir:="/home/nova/nova/src/ros/rover/rover_description"')
        model = PathJoinSubstitution(['/home/nova/nova/src/ros/rover/rover_description', 'banksia', 'urdf', 'rover.urdf.xacro'])
    urdf_value = ParameterValue(Command(['xacro ', model, ' '] + xacro_args), value_type=str)

    return [
        Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            parameters=[{'robot_description': urdf_value}]
        ),
        # Launch joint states for arm
        Node(
            package='joint_state_publisher',
            executable='joint_state_publisher',
            namespace='',
            output='screen',
            emulate_tty=True,
            parameters=[{
                'source_list': ['/arm/joint_states']
            }]
        ),

        Node(
            package='rviz2',
            namespace='',
            executable='rviz2',
            name='rviz2',
            arguments=['-d', [PathJoinSubstitution([arm_bringup_dir, 'rviz', 'arm.rviz'])], '-f', fixed_frame],
            condition=IfCondition(rviz),
        ),
    ]


def generate_launch_description():
    rover_description_dir = FindPackageShare('rover_description')

    declared_arguments = [
        DeclareLaunchArgument(
            name='gazebo', 
            default_value='True',
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
            name='arm',
            default_value='false',
            description='whether to launch arm',
        ),
        DeclareLaunchArgument(
            name='old_arm',
            default_value='True',
            description='whether to launch the old arm (for new arm software)',
        ),
        DeclareLaunchArgument(
            name='use_mock_hardware',
            default_value='false',
            description='whether to use mock hardware for hardware interfaces',
        ),
        DeclareLaunchArgument(
            name='rviz',
            default_value='false',
            description='whether to launch rviz',
        ),
        DeclareLaunchArgument(
            name='local',
            default_value='False',
            description='whether to use the local rover_description source directory instead of the nix store.',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )