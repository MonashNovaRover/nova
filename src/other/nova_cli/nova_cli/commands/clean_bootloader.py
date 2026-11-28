"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Clean out the old nixos generations in the boot entries
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
EXAMPLES:
  nova clean-bootloader 
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


class CleanBootloaderCommand(Command):
    """Implements 'nova clean-bootloader' command as wrapper for /run/current-system/bin/switch-to-configuration"""

    @staticmethod
    def add_parser(subparsers):
        parser = subparsers.add_parser(
            'clean-bootloader',
            help='Clean out the old nixos generations in the boot entries',
            description='Wrapper for /run/current-system/bin/switch-to-configuration',
        )

        parser.add_argument(
            'configuration',
            nargs='?',
            choices=('boot', 'switch', 'test', 'dry-activate'),
            default='boot',
            help='Nixos bootloader configuration (default: boot)'
        )

        return parser

    @staticmethod
    def execute(args):

        cmd = [
            'sudo',
            '/run/current-system/bin/switch-to-configuration',
            args.configuration
        ] + args.extra_args

        print(f"Running: {' '.join(cmd)}", file=sys.stderr)

        try:
            result = subprocess.run(cmd)
            return result.returncode
        except FileNotFoundError:
            print("Error: /run/current-system/bin/switch-to-configuration not found", file=sys.stderr)
            return 1

