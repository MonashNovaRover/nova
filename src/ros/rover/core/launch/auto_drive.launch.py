"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the rover to start all
   rover control scripts.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - control/drive/drive_inputs      [drive_cmd]
  - control/drive/driver            [driver]
  - tf2 static transforms
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	core
CREATION:	15/12/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

# Include the required launch parameters
from launch import LaunchDescription
from launch.substitutions import LaunchConfiguration, PythonExpression
from launch.conditions import IfCondition, UnlessCondition
from launch.actions import DeclareLaunchArgument, RegisterEventHandler
from launch.event_handlers import OnProcessExit
from launch.launch_context import LaunchContext
from launch.events.process.process_exited import ProcessExited
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_path


# For re-launching path_planner if it crashes
def planner_description():
    return Node(
        package="autonomous",
        executable="path_planner.py",
        output='screen',
        emulate_tty=True
    )


def on_exit_restart(event: ProcessExited, context:LaunchContext):
    if event.returncode != 0:
        print(f"\n\nProcess[{event.action.name}] exited, pid: {event.pid}, return code: {event.returncode}")
        if 'planner' in event.action.name:
            print("Restarting planner...")
            return planner_description()


# Generate the launch file with all inputs
def generate_launch_description():
    # Get the path to the parameters files in core package
    core_params_path = get_package_share_path('core') / "params"

    from_rosbag = LaunchConfiguration('from_rosbag')
    from_sim = LaunchConfiguration('sim_cameras')

    from_rosbag_arg = DeclareLaunchArgument(
        name='from_rosbag',
        default_value='False',
        description="Set to 'True' to run autonomous algorithms on camera data we will play from a rosbag"
    )

    from_sim_arg = DeclareLaunchArgument(
        name='sim_cameras',
        default_value='False',
        description="Set to 'True' to run autonomous algorithms with simulated ar tags"
    )

    # autonomous nodes
    map_launch = Node(
        package="autonomous",
        executable="mapper.py",
        output="screen",
        parameters=[core_params_path / "auto_params.yaml"],
        emulate_tty=True
    )
    planner_launch = Node(
        package="autonomous",
        executable="path_planner.py",
        output="screen",
        parameters=[core_params_path / "auto_params.yaml"],
        emulate_tty=True,

    )
    depth_cam_launch = Node(
        package="autonomous",
        executable="depth_camera.py",
        output="screen",
        parameters=[core_params_path / "auto_params.yaml"],
        emulate_tty=True,
        condition=UnlessCondition(PythonExpression(
            [from_rosbag, " or ", from_sim]
        ))
    )

    marker_vis_node = Node(
        package='autonomous',
        executable='marker_vis.py',
        output='screen',
        emulate_tty=True
    )

    stamp_converter_launch = Node(
        package="autonomous",
        executable="stamp_converter.py",
        output="screen",
        parameters=[core_params_path / "auto_params.yaml"],
        emulate_tty=True,
        condition=IfCondition(from_rosbag)
    )
    ar_sim_node = Node(
        package="autonomous",
        executable="ar_tag_sim.py",
        output="screen",
        parameters=[core_params_path / "auto_params.yaml"],
        emulate_tty=True,
        condition=IfCondition(from_sim)
    )
    controller_launch = Node(
        package="autonomous",
        executable="controller.py",
        output="screen",
        parameters=[core_params_path / "auto_params.yaml"],
        emulate_tty=True
    )
    goal_selector_launch = Node(
        package="autonomous",
        executable="goal_selector.py",
        output="screen",
        parameters=[core_params_path / "auto_params.yaml"],
        emulate_tty=True
    )

    return LaunchDescription([
        from_rosbag_arg,
        from_sim_arg,
        map_launch,
        planner_launch,
        depth_cam_launch,
        marker_vis_node,
        ar_sim_node,
        stamp_converter_launch,
        controller_launch,
        goal_selector_launch,
        RegisterEventHandler(event_handler=OnProcessExit(on_exit=on_exit_restart))
    ])
