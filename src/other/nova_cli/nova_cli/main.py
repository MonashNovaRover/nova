#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Main entry point for the nova CLI. Handles argument parsing, global
flag extraction (allowing flags like -b anywhere), build path
validation, and command dispatch.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        nova_cli
AUTHOR(S):      Felicity Matthews
CREATION:       06/07/2026
EDITED:         09/07/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import argparse
import sys
import os
from pathlib import Path

# IMPORTANT: Import before argparse setup for completion to work
try:
    import argcomplete
    HAS_ARGCOMPLETE = True
except ImportError:
    HAS_ARGCOMPLETE = False

from nova_cli.commands import COMMANDS
from nova_cli.utils import validate_build_path, list_available_builds
from nova_cli import __version__


def create_parser():
    """Create the main argument parser"""
    parser = argparse.ArgumentParser(
        prog='nova',
        description='Nova ROS2 wrapper CLI tool',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
  nova launch science urc
  nova launch science          # Defaults to science.launch.py
  nova launch teleop drive
  nova launch auto sim -b auto # -b flag can go anywhere
  nova run science kiln
  nova launch drive auto:=True -b master
        '''
    )

    # Global options
    parser.add_argument(
        '-b', '--build',
        default='active',
        help='Build to use (default: active). Available: master, auto, arm, drive, etc.'
    )

    # Add version
    parser.add_argument('--version', action='version', version=f'%(prog)s {__version__}')

    # Subcommands
    subparsers = parser.add_subparsers(dest='command', help='Available commands')

    # Register all commands
    for cmd_name, cmd_class in COMMANDS.items():
        cmd_class.add_parser(subparsers)

    return parser


def extract_global_flags(argv):
    """
    Extract global flags from argv, allowing them to appear anywhere.

    Returns (global_flags_dict, filtered_argv)

    This is extensible - just add new flags to the GLOBAL_FLAGS dict.
    """
    GLOBAL_FLAGS = {
        '-b': ('build', True),      # Flag: (dest_name, has_value)
        '--build': ('build', True),
    }

    global_values = {'build': 'active'}  # Defaults
    filtered = []
    i = 0

    while i < len(argv):
        arg = argv[i]

        if arg in GLOBAL_FLAGS:
            dest, has_value = GLOBAL_FLAGS[arg]
            if has_value:
                if i + 1 < len(argv):
                    global_values[dest] = argv[i + 1]
                    i += 2
                else:
                    print(f"Error: {arg} requires a value", file=sys.stderr)
                    sys.exit(1)
            else:
                global_values[dest] = True
                i += 1
        else:
            filtered.append(arg)
            i += 1

    return global_values, filtered


def main():
    """Main entry point"""
    # Extract global flags that can appear anywhere
    # This allows: launch science urc -b master
    global_flags, filtered_argv = extract_global_flags(sys.argv[1:])

    # Reconstruct sys.argv for argparse
    sys.argv = [sys.argv[0]] + filtered_argv

    parser = create_parser()

    # Enable bash completion if available
    if HAS_ARGCOMPLETE:
        argcomplete.autocomplete(parser)

    args, extra_args = parser.parse_known_args()

    # Apply extracted global flags
    for key, value in global_flags.items():
        setattr(args, key, value)

    # Validate command was provided
    if not args.command:
        parser.print_help()
        sys.exit(1)

    # Validate build path
    build_path = validate_build_path(args.build)
    if not build_path:
        available = list_available_builds()
        print(f"Error: Build '{args.build}' not found.", file=sys.stderr)
        if available:
            print(f"Available builds: {', '.join(available)}", file=sys.stderr)
        else:
            print(f"No builds found in ~/Builds/", file=sys.stderr)
        sys.exit(1)

    # Store build path and extra args for command
    args.build_path = build_path
    args.extra_args = extra_args

    # Execute command
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
