"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Executes terminal launch scripts from the build's launch directory.
Scripts are located at ~/Builds/<build>/launch/.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
EXAMPLES:
  nova start run-gui            # Run the GUI launch script
  nova start run-drive          # Run the drive subsystem script
  nova start run-auto           # Run the autonomy script
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        nova_cli
AUTHOR(S):      Felicity Matthews
CREATION:       06/07/2026
EDITED:         09/07/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import subprocess
import sys

from nova_cli.commands.base import Command
from nova_cli.build_utils import add_build_argument, validate_build_arg
from nova_cli import ros2_utils


class StartCommand(Command):
    """Implements 'nova start' command to run terminal launch scripts"""

    @staticmethod
    def complete_script(prefix, parsed_args, **kwargs):
        """Complete script names from launch directory."""
        build_path = ros2_utils.get_build_path()
        launch_dir = build_path / "launch"
        scripts = ros2_utils.list_scripts(launch_dir)
        return [s for s in scripts if s.startswith(prefix)]

    @staticmethod
    def add_parser(subparsers):
        parser = subparsers.add_parser(
            'start',
            help='Run a terminal launch script',
            description='Execute launch scripts from ~/Builds/<build>/launch/'
        )

        add_build_argument(parser)

        script_arg = parser.add_argument(
            'script',
            help='Script name (e.g., run-gui, run-drive, run-auto)'
        )
        script_arg.completer = StartCommand.complete_script

        return parser

    @staticmethod
    def execute(args):
        if (err := validate_build_arg(args)) is not None:
            return err

        launch_dir = args.build_path / "launch"

        if not launch_dir.exists():
            print(f"Error: Launch directory not found: {launch_dir}", file=sys.stderr)
            return 1

        script_path = launch_dir / args.script

        if not script_path.exists() or not script_path.is_file():
            available = ros2_utils.list_scripts(launch_dir)
            print(f"Error: Script '{args.script}' not found.", file=sys.stderr)

            if available:
                from nova_cli.utils import fuzzy_match
                suggestions = fuzzy_match(args.script, available)
                if suggestions:
                    print(f"Did you mean: {', '.join(suggestions)}?", file=sys.stderr)
                else:
                    print(f"Available scripts: {', '.join(available[:10])}", file=sys.stderr)
                    if len(available) > 10:
                        print(f"... and {len(available) - 10} more", file=sys.stderr)
            return 1

        # Build command
        cmd = [str(script_path)] + args.extra_args
        print(f"Running: {' '.join(cmd)}", file=sys.stderr)

        result = subprocess.run(cmd)
        return result.returncode
