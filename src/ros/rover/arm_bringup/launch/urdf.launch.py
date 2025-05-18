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

def launch_setup(context, *args, **kwargs):
    angle = LaunchConfiguration('angle').perform(context)
    gazebo = LaunchConfiguration('gazebo').perform(context)
    model = LaunchConfiguration('model').perform(context)
    robot_name = LaunchConfiguration('robot_name').perform(context)
    arm = LaunchConfiguration('arm').perform(context)
    old_arm = LaunchConfiguration('old_arm').perform(context)
    use_mock_hardware = LaunchConfiguration('use_mock_hardware').perform(context)
    use_local_mesh = LaunchConfiguration('use_local_mesh').perform(context)

    if arm.lower() in ['true', '1', 'yes']:
        arm += ' auto_camera:=false'

    if old_arm.lower() in ['true', '1', 'yes']:
        old_arm += ' auto_camera:=false'

    return [
        Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            parameters=[{'robot_description': 
                ParameterValue(Command(['xacro ', model, ' ', 'gazebo:=', gazebo, ' ', 'robot_name:=', robot_name, ' ', 'angle:=', angle, ' ', 'arm:=', arm, ' ', 'old_arm:=', old_arm, ' ', 'use_mock_hardware:=', use_mock_hardware, ' ', 'use_local_mesh:=', use_local_mesh]), value_type=str)
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
            name='use_local_mesh',
            default_value='False',
            description='Use local mesh paths instead of nix store paths',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )