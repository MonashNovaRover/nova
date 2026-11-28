"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Clean out the nix store
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
EXAMPLES:
  nova hard-link 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        nova_cli
AUTHOR(S):      Brandon Chung
CREATION:       28/11/2026
EDITED:         28/11/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import subprocess
import sys

from nova_cli.commands.base import Command


class HardLinkCommand(Command):
    """Implements 'nova hard-link' command as wrapper for nix-store --optimise"""

    @staticmethod
    def add_parser(subparsers):
        parser = subparsers.add_parser(
            'hard-link',
            help='Optimise the nix store by deduplicating packages',
            description='Wrapper for nix-store --optimise'
        )

        return parser

    @staticmethod
    def execute(args):

        cmd = [
            'nix-store',
            '--optimise'
        ] + args.extra_args

        print(f"Running: {' '.join(cmd)}", file=sys.stderr)

        try:
            result = subprocess.run(cmd)
            return result.returncode
        except FileNotFoundError:
            print("Error: nix-store not found in PATH", file=sys.stderr)
            return 1

