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
import random
import sys
from pathlib import Path

# IMPORTANT: Import before argparse setup for completion to work
try:
    import argcomplete
    HAS_ARGCOMPLETE = True
except ImportError:
    HAS_ARGCOMPLETE = False

from nova_cli.commands import COMMANDS
from nova_cli import __version__


def print_random_meme():
    """Print a random meme from the memes directory."""
    memes_dir = Path(__file__).parent / "memes"
    if memes_dir.exists():
        meme_files = list(memes_dir.glob("*.txt"))
        if meme_files:
            meme = random.choice(meme_files).read_text()
            print(meme)


def create_parser():
    """Create the main argument parser"""
    parser = argparse.ArgumentParser(
        prog='nova',
        description="Monash Nova Rover's very own CLI that does all of the every day commands that you need.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
  nova launch science urc           Launch science_bringup/urc.launch.py
  nova run science kiln             Run science/kiln.py node
  nova start gui                    Run ~/Builds/active/launch/run-gui script
  nova build master                 Build workspace to ~/Builds/master
  nova env set build master         Switch active build to master
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
    # Print meme only for top-level help (nova -h), not subcommand help (nova launch -h)
    if (len(sys.argv) == 2 and sys.argv[1] in ('-h', '--help')) or len(sys.argv) == 1:
        print_random_meme()

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
