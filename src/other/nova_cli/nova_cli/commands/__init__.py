"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Command registry for nova CLI. Imports and registers all available
commands in the COMMANDS dictionary.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        nova_cli
AUTHOR(S):      Felicity Matthews
CREATION:       06/07/2026
EDITED:         09/07/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

from nova_cli.commands.build import BuildCommand
from nova_cli.commands.env import EnvCommand
from nova_cli.commands.launch import LaunchCommand
from nova_cli.commands.rebuild_main import RebuildMasterCommand
from nova_cli.commands.run import RunCommand
from nova_cli.commands.sleuth import SleuthCommand
from nova_cli.commands.start import StartCommand

# Command registry - add new commands here
COMMANDS = {
    'build': BuildCommand,
    'env': EnvCommand,
    'launch': LaunchCommand,
    'rebuild-main': RebuildMasterCommand,
    'run': RunCommand,
    'sleuth': SleuthCommand,
    'start': StartCommand,
}
