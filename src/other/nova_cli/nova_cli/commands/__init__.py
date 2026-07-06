"""Command registry for nova CLI"""

from nova_cli.commands.launch import LaunchCommand
from nova_cli.commands.run import RunCommand

# Command registry - add new commands here
COMMANDS = {
    'launch': LaunchCommand,
    'run': RunCommand,
}
