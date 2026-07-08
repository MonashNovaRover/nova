"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Command registry for nova CLI. Imports and registers all available
commands in the COMMANDS dictionary.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        nova_cli
AUTHOR(S):      Felicity Matthews
CREATION:       26/01/26
EDITED:         26/07/09
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

from nova_cli.commands.env import EnvCommand
from nova_cli.commands.launch import LaunchCommand
from nova_cli.commands.run import RunCommand
from nova_cli.commands.sleuth import SleuthCommand
from nova_cli.commands.start import StartCommand

# Command registry - add new commands here
COMMANDS = {
    'env': EnvCommand,
    'launch': LaunchCommand,
    'run': RunCommand,
    'sleuth': SleuthCommand,
    'start': StartCommand,
}
