'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the rover to start the
   specified CAN bus with logging
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PROCESSES:
  - candump
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CREATION:   27/02/2026
EDITED:     28/02/2026
EDITED BY: Jonathan Jia
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from datetime import datetime

import launch.logging
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction, ExecuteProcess, \
    LogInfo, RegisterEventHandler, GroupAction
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch.event_handlers import OnProcessExit
import subprocess
from os.path import expanduser


def start_can(bus: str, bitrate: str):
    logger = launch.logging.get_logger()

    def can_is_up():
        try:
            result = subprocess.run(["ip", "link", "show", bus],
                                    capture_output=True, text=True, check=True)
            return "UP" in result.stdout
        except subprocess.CalledProcessError:
            return False

    if not can_is_up():
        logger.info(f"Starting {bus}... (may require password)")
        try:
            subprocess.run(["can", "start", bus, bitrate],
                           capture_output=True, text=True, check=True)
            logger.info(f"{bus} started successfully")
        except subprocess.CalledProcessError as e:
            logger.error(f"Failed to start {bus}: {e}")
    else:
        return logger.warning(f"{bus} is already running "
                              f"(check bitrate; may differ from requested bitrate: {bitrate})")

def launch_setup(context, *args, **kwargs):
    bus = LaunchConfiguration('bus').perform(context)
    bitrate = LaunchConfiguration('bitrate').perform(context)
    log_can = LaunchConfiguration('log_can').perform(context)
    log_dir = LaunchConfiguration('log_dir').perform(context)
    log_dir_expanded = expanduser(log_dir)
    can_log_name = LaunchConfiguration('can_log_name').perform(context)

    logger = launch.logging.get_logger()

    # start can bus
    start_can(bus, bitrate)

    # create log file directory
    if log_can.lower() == "true":
        try:
            subprocess.run(['mkdir', '-p', log_dir_expanded],
                           capture_output=True, text=True, check=True)
        except subprocess.CalledProcessError as e:
            logger.error(f"Failed to create CAN log directory {log_dir_expanded}: {e}")

    # generate log file path
    time_stamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    if can_log_name:
        log_file_name = f"{can_log_name}_{bus}_{time_stamp}.log"
    else:
        log_file_name = f"{bus}_{time_stamp}.log"
    log_path = PathJoinSubstitution([log_dir_expanded, log_file_name])

    can_logger = ExecuteProcess(
        cmd=[
            "candump",
            bus,
            "-f",
            log_path
        ],
        output="screen",
    )

    return [
        GroupAction(
            condition=IfCondition(log_can),
            actions=[
                LogInfo(msg=["Logging ", bus," to: ", log_path]),
                can_logger,
                RegisterEventHandler(OnProcessExit(
                    target_action=can_logger,
                    on_exit=LogInfo(msg="ERROR: CAN logging stopped") # use LogError when available
                ))
            ]
        )
    ]

def generate_launch_description():
    
    declared_arguments = [
        DeclareLaunchArgument(
            name='bus',
            default_value='can1',
            description='name of the CAN bus to start/log',
            # reminder to use can instead of vcan
            choices=[f'can{n}' for n in range(0, 10)],
        ),
        DeclareLaunchArgument(
            name='bitrate',
            default_value="200000",
            description='bitrate of the CAN bus'
        ),
        DeclareLaunchArgument(
            name='log_can',
            default_value='true',
            description='log CAN messages sent on the CAN bus'
        ),
        DeclareLaunchArgument(
            name='log_dir',
            default_value='~/.nova/log',
            description='directory where CAN message log files are stored'
        ),
        DeclareLaunchArgument(
            name='log_name',
            default_value="",
            description='optional prefix for log file name'
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )