"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Builds the Nova workspace to a named output directory. Wraps nom-build
with automatic output path handling.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
EXAMPLES:
  nova build main             # Build to ~/Builds/main
  nova build auto               # Build to ~/Builds/auto
  nova build test --packages-select science  # Pass extra args
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        nova_cli
AUTHOR(S):      Felicity Matthews
CREATION:       09/07/2026
EDITED:         09/07/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import subprocess
import sys
from pathlib import Path

from nova_cli.commands.base import Command
from nova_cli.build_utils import list_available_builds


class BuildCommand(Command):
    """Implements 'nova build' command as wrapper for nom-build"""

    @staticmethod
    def add_parser(subparsers):
        parser = subparsers.add_parser(
            'build',
            help='Build the workspace to ~/Builds/<name>',
            description='Wrapper for ws-build -o ~/Builds/<buildname>',
            add_help=False,  # Let ws-build handle --help
        )

        buildname_arg = parser.add_argument(
            'buildname',
            help='Build output name (e.g., main, auto, arm)'
        )
        buildname_arg.completer = BuildCommand.complete_buildname

        return parser

    @staticmethod
    def execute(args):
        builds_dir = Path.home() / "Builds"
        output_path = builds_dir / args.buildname

        cmd = [
            'bash', '-ic', # use bash to get the alias
            " ".join([
                'ws-build',
                '-o',
                str(output_path)
                ] + args.extra_args
            )
        ]

        print(f"Running: {' '.join(cmd)}", file=sys.stderr)

        try:
            result = subprocess.run(cmd)
            return result.returncode
        except FileNotFoundError:
            print("Error: nom-build not found in PATH", file=sys.stderr)
            return 1

    @staticmethod
    def complete_buildname(prefix, parsed_args, **kwargs):
        """Complete build names from existing builds."""
        builds = list_available_builds()
        return [b for b in builds if b.startswith(prefix)]
