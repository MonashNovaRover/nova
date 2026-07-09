#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Main entry point for the nova CLI. Handles argument parsing and
command dispatch.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        nova_cli
AUTHOR(S):      Felicity Matthews
CREATION:       06/07/2026
EDITED:         09/07/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import argparse
import sys

# IMPORTANT: Import before argparse setup for completion to work
try:
    import argcomplete
    HAS_ARGCOMPLETE = True
except ImportError:
    HAS_ARGCOMPLETE = False

from nova_cli.commands import COMMANDS
from nova_cli import __version__


def create_parser():
    """Create the main argument parser"""
    parser = argparse.ArgumentParser(
        prog='nova',
        description='CLI for Nova Rover ROS2 development and operations.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
  nova launch science urc           Launch science_bringup/urc.launch.py
  nova launch teleop drive          Launch teleop_drive_bringup/teleop.launch.py
  nova run science kiln             Run science/kiln.py node
  nova start run-gui                Run ~/Builds/active/launch/run-gui script
  nova build master                 Build workspace to ~/Builds/master
  nova env set build master         Switch active build to master
  nova env status                   Show current environment configuration

Use -b <build> on launch/run/start to use a specific build:
  nova launch science -b master     Use ~/Builds/master instead of active
        '''
    )

    # Add version
    parser.add_argument('--version', action='version', version=f'%(prog)s {__version__}')

    # Subcommands
    subparsers = parser.add_subparsers(dest='command', help='Available commands')

    # Register all commands
    for cmd_name, cmd_class in COMMANDS.items():
        cmd_class.add_parser(subparsers)

    return parser


def main():
    """Main entry point"""
    parser = create_parser()

    # Enable bash completion if available
    if HAS_ARGCOMPLETE:
        argcomplete.autocomplete(parser)

    args, extra_args = parser.parse_known_args()

    # Validate command was provided
    if not args.command:
        parser.print_help()
        sys.exit(1)

    # Store extra args for command
    args.extra_args = extra_args

    # Execute command (build validation now happens in command)
    try:
        cmd_class = COMMANDS[args.command]
        sys.exit(cmd_class.execute(args))
    except KeyboardInterrupt:
        print("\nInterrupted by user", file=sys.stderr)
        sys.exit(130)
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()
