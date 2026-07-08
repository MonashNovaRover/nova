"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Passthrough command to can_sleuth for CAN bus tracing and device
emulation. All arguments are forwarded directly to can_sleuth.
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


class SleuthCommand(Command):
    """Implements 'nova sleuth' command as passthrough to can_sleuth"""

    # TODO: For proper integration, can_sleuth should expose a run() function:
    #
    # def run(system_configs: list, output_names: list = None):
    #     """
    #     Run can_sleuth programmatically.
    #     Args:
    #         system_configs: List of (system_name, interface, emulate) tuples
    #         output_names: List of output names, or None for default
    #     """
    #
    # Then this command could use argparse properly:
    #   - parser.add_argument('systems', nargs='+', choices=allSystems.keys())
    #   - parser.add_argument('-e', '--emulate', action='store_true')
    #   - parser.add_argument('-i', '--interface')
    #   - parser.add_argument('-o', '--output', choices=allOutputs.keys())
    #
    # And call: run_sleuth(system_configs, outputs)

    @staticmethod
    def add_parser(subparsers):
        parser = subparsers.add_parser(
            'sleuth',
            help='CAN bus tracer/emulator (passthrough to can_sleuth)',
            description='Trace or emulate CAN bus devices. All arguments passed to can_sleuth.',
            add_help=False,  # Let can_sleuth handle --help
        )
        return parser

    @staticmethod
    def execute(args):
        # Pass all extra args directly to can_sleuth
        cmd = ['can_sleuth'] + args.extra_args

        print(f"Running: {' '.join(cmd)}", file=sys.stderr)

        try:
            result = subprocess.run(cmd)
            return result.returncode
        except FileNotFoundError:
            print("Error: can_sleuth not found in PATH", file=sys.stderr)
            return 1
