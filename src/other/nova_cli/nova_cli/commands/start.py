"""Start command - run terminal launch scripts"""
import os
import subprocess
import sys

from nova_cli.commands.base import Command
from nova_cli.completion import complete_scripts


class StartCommand(Command):
    """Implements 'nova start' command to run terminal launch scripts"""

    @staticmethod
    def add_parser(subparsers):
        parser = subparsers.add_parser(
            'start',
            help='Run a terminal launch script',
            description='Execute launch scripts from ~/Builds/<build>/launch/'
        )

        script_arg = parser.add_argument(
            'script',
            help='Script name (e.g., run-gui, run-drive, run-auto)'
        )
        script_arg.completer = complete_scripts

        return parser

    @staticmethod
    def execute(args):
        launch_dir = args.build_path / "launch"

        if not launch_dir.exists():
            print(f"Error: Launch directory not found: {launch_dir}", file=sys.stderr)
            return 1

        script_path = launch_dir / args.script

        if not script_path.exists() or not script_path.is_file():
            available = StartCommand._list_scripts(launch_dir)
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

    @staticmethod
    def _list_scripts(launch_dir):
        """List available scripts"""
        if not launch_dir.exists():
            return []

        scripts = []
        for f in launch_dir.iterdir():
            if f.is_file() and os.access(f, os.X_OK):
                scripts.append(f.name)
        return sorted(scripts)
