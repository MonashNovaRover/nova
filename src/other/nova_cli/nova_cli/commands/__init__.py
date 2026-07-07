"""Command registry for nova CLI"""

from nova_cli.commands.env import EnvCommand
from nova_cli.commands.launch import LaunchCommand
from nova_cli.commands.run import RunCommand

# Command registry - add new commands here
COMMANDS = {
    'env': EnvCommand,
    'launch': LaunchCommand,
    'run': RunCommand,
}
