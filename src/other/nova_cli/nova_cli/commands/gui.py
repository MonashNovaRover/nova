"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Runs the gui and its submodules.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
EXAMPLES:
  nova gui                      # Runs ~/Builds/active/gui-serve
  nova gui rosbridge            # Runs ~/Builds/active/bin/ros2 launch rosbridge_server rosbridge_websocket_launch.xml 
  nova gui tileserver           # Runs ~/Builds/active/bin/mbtileserver -p 8080 --missing-image-tile-404 -d ~/maps
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        nova_cli
AUTHOR(S):      Anthony Lew
CREATION:       05/10/2026
EDITED:         05/10/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import subprocess
import sys
from pathlib import Path

from nova_cli.commands.base import Command
from nova_cli.build_utils import add_build_argument, validate_build_arg
from nova_cli import ros2_utils


class RunCommand(Command):
    """Implements 'nova gui' command"""

    @staticmethod
    def add_parser(subparsers):
        """Add this command to the argument parser"""
        parser = subparsers.add_parser(
            'gui',
            help='Run the Nova GUI or one of its services',
            description='Run the GUI, rosbridge, or map tile server'
        )

        add_build_argument(parser)
        parser.add_argument(
            'service',
            nargs='?',
            choices=('dir', 'shell', 'rosbridge', 'tileserver'),
            default='gui',
            help='Service to run (default: gui)'
        )

        return parser

    @staticmethod
    def execute(args):
        """Execute the command"""
        if (err := validate_build_arg(args)) is not None:
            return err

        build_path = Path(args.build_path)
        extra_args = list(args.extra_args)

        if args.service == 'rosbridge':
            print(f"Running: {args.service}", file=sys.stderr)
            return ros2_utils.run_ros2_command(
                build_path,
                [
                    'launch',
                    'rosbridge_server',
                    'rosbridge_websocket_launch.xml',
                ] + extra_args
          )

        if args.service == 'tileserver':
            command = [
                build_path / 'bin' / 'mbtileserver',
                '-p', '8080',
                '--missing-image-tile-404',
                '-d',
                str(Path.home() / 'maps'),
            ] + extra_args
            executable = command[0]
            if not executable.is_file():
                print(f"Error: {executable} not found:", file=sys.stderr)
                return 1

            printable_command = ' '.join(str(arg) for arg in command)
            print(f"Running: {printable_command}", file=sys.stderr)
            return subprocess.run([str(arg) for arg in command]).returncode

        if args.service == 'shell':
            command = [
                'bash', '-ic', # use bash to get the alias
                f'nova-shell -A pkgs.ros.nova-gui {" ".join(args.extra_args)}'
            ]
            return subprocess.run([str(arg) for arg in command]).returncode

        else:
            command = [
                build_path / 'bin' / 'gui-serve', '5173'
            ] + extra_args
            print("https://localhost:5173")

            executable = command[0]
            if not executable.is_file():
                print(f"Error: GUI executable not found: {executable}", file=sys.stderr)
                return 1

            printable_command = ' '.join(str(arg) for arg in command)
            print(f"Running: {printable_command}", file=sys.stderr)
            return subprocess.run([str(arg) for arg in command]).returncode
