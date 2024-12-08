import os
from ament_index_python.packages import get_package_share_directory
import launch
import launch_ros.actions


def generate_launch_description():
    # Launch configurations
    joystick = launch.substitutions.LaunchConfiguration("joystick", default="xbox")
    joy_device = launch.substitutions.LaunchConfiguration(
        "joy_dev", default="/dev/input/js0"
    )
    joy_vel = launch.substitutions.LaunchConfiguration("joy_vel", default="cmd_vel")
    config_filepath = launch.substitutions.LaunchConfiguration("config_filepath")

    # Path substitution for config file
    default_config_path = launch.substitutions.LaunchConfiguration(
        "config_filepath",
        default=[
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
    )

    # Nodes
    joy_node = launch_ros.actions.Node(
        package="joy",
        executable="joy_node",
        name="joy_node",
        parameters=[
            {
                "dev": joy_device,
                "deadzone": 0.1,
                "autorepeat_rate": 20.0,
            }
        ],
    )

    teleop_node = launch_ros.actions.Node(
        package="teleop_drive_joy",
        executable="teleop_drive_joy_node",
        name="teleop_drive_joy_node",
        parameters=[config_filepath],
        remappings=[
            ("/cmd_vel", joy_vel),
        ],
    )

    # Launch Description
    return launch.LaunchDescription(
        [
            # Log Information
            launch.actions.LogInfo(msg=["Joystick Loaded: ", joystick]),
            # Declare Launch Arguments
            launch.actions.DeclareLaunchArgument("joystick", default_value="xbox"),
            launch.actions.DeclareLaunchArgument(
                "joy_dev", default_value="/dev/input/js0"
            ),
            launch.actions.DeclareLaunchArgument("joy_vel", default_value="cmd_vel"),
            launch.actions.DeclareLaunchArgument(
                "config_filepath", default_value=default_config_path
            ),
            # Add Nodes
            joy_node,
            teleop_node,
        ]
    )
