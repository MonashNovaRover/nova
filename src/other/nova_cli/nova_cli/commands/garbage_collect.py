"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Clean out the nix store
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
EXAMPLES:
  nova garbage-collect 
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


class GarbageCollectCommand(Command):
    """Implements 'nova garbage-collect' command as wrapper for nix-collect-garbage"""

    @staticmethod
    def add_parser(subparsers):
        parser = subparsers.add_parser(
            'garbage-collect',
            help='Garbage collect the nix store',
            description='Wrapper for nix-collect-garbage'
        )

        parser.add_argument(
            '--delete-older-than',
            metavar='DURATION',
            default='180d',
            help='Delete generations older than DURATION (default: 180d)'
        )

        return parser

    @staticmethod
    def execute(args):

        cmd = [
            'nix-collect-garbage',
            '--delete-older-than',
            args.delete_older_than,
        ] + args.extra_args

        print(f"Running: {' '.join(cmd)}", file=sys.stderr)

        try:
            result = subprocess.run(cmd)
            return result.returncode
        except FileNotFoundError:
            print("Error: nix-collect-garbage not found in PATH", file=sys.stderr)
            return 1

