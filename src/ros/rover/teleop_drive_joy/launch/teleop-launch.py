import os

from ament_index_python.packages import get_package_share_directory

import launch
import launch_ros.actions


def generate_launch_description():
    joystick = launch.substitutions.LaunchConfiguration("joystick")
    joy_dev = launch.substitutions.LaunchConfiguration("joy_dev")
    config_filepath = launch.substitutions.LaunchConfiguration("config_filepath")

    return launch.LaunchDescription(
        [
            launch.actions.DeclareLaunchArgument("joy_vel", default_value="cmd_vel"),
            launch.actions.DeclareLaunchArgument(
                "joystick", default_value="xbox"
            ),  # xbox, ps5, nintendo
            launch.actions.DeclareLaunchArgument(
                "joy_dev", default_value="/dev/input/js0"
            ),
            launch.actions.DeclareLaunchArgument(
                "config_filepath",
                default_value=[
                    launch.substitutions.TextSubstitution(
                        text=os.path.join(
                            get_package_share_directory("teleop_drive_joy"),
                            "config",
                            "",
                        )
                    ),
                    joystick,
                    launch.substitutions.TextSubstitution(text=".config.yaml"),
                ],
            ),
            launch_ros.actions.Node(
                package="joy",
                executable="joy_node",
                name="joy_node",
                parameters=[
                    {
                        "dev": joy_dev,
                        "deadzone": 0.1,
                        "autorepeat_rate": 20.0,
                    }
                ],
            ),
            launch_ros.actions.Node(
                package="teleop_drive_joy",
                executable="teleop_drive_joy_node",
                name="teleop_drive_joy_node",
                parameters=[config_filepath],
                remappings={
                    ("/cmd_vel", launch.substitutions.LaunchConfiguration("joy_vel"))
                },
            ),
        ]
    )
