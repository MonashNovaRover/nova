"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Checkout master branch, pull latest changes, and rebuild.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
EXAMPLES:
  nova rebuild-master              # Update and rebuild master
  nova rebuild-master --stash      # Auto-stash uncommitted changes
  nova rebuild-master --verbose    # Pass --verbose to nom-build
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        nova_cli
AUTHOR(S):      Nova Team
CREATION:       09/07/2026
EDITED:         09/07/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import sys
import subprocess
from pathlib import Path

from nova_cli.commands.base import Command


class RebuildMasterCommand(Command):
    """Implements 'nova rebuild-master' command"""

    @staticmethod
    def add_parser(subparsers):
        """Add rebuild-master subcommand parser"""
        parser = subparsers.add_parser(
            'rebuild-master',
            help='Checkout master, pull latest changes, and rebuild',
            description='Checkout master branch in ~/nova, pull latest changes, '
                       'and build to ~/Builds/master'
        )

        parser.add_argument(
            '--stash',
            action='store_true',
            help='Automatically stash uncommitted changes before checkout'
        )

        return parser

    @staticmethod
    def execute(args):
        """Execute rebuild-master command"""
        # Define paths
        nova_dir = Path.home() / "nova"
        builds_dir = Path.home() / "Builds"
        output_path = builds_dir / "master"

        # Step 1: Validate nova directory exists
        if not nova_dir.exists():
            print("Error: Nova directory not found at ~/nova", file=sys.stderr)
            return 1

        # Step 2: Check for uncommitted changes
        print("Checking for uncommitted changes...", file=sys.stderr)
        cmd = ['git', '-C', str(nova_dir), 'status', '--porcelain']
        result = subprocess.run(cmd, capture_output=True, text=True)

        if result.returncode != 0:
            print("Error: Failed to check git status", file=sys.stderr)
            return 1

        has_changes = bool(result.stdout.strip())

        if has_changes:
            if args.stash:
                print("Stashing uncommitted changes...", file=sys.stderr)
                cmd = ['git', '-C', str(nova_dir), 'stash', '-u']
                result = subprocess.run(cmd)
                if result.returncode != 0:
                    print("Error: Failed to stash changes", file=sys.stderr)
                    return 1
            else:
                print("Error: Uncommitted changes detected in ~/nova", file=sys.stderr)
                print("Please commit or stash your changes before updating:", file=sys.stderr)
                print("  git stash", file=sys.stderr)
                print("  git commit -am 'message'", file=sys.stderr)
                print("\nOr run with --stash flag to automatically stash changes:", file=sys.stderr)
                print("  nova rebuild-master --stash", file=sys.stderr)
                return 1

        # Step 3: Checkout master
        print("Checking out master branch...", file=sys.stderr)
        cmd = ['git', '-C', str(nova_dir), 'checkout', 'master']
        result = subprocess.run(cmd)

        if result.returncode != 0:
            print("Error: Failed to checkout master branch", file=sys.stderr)
            return 1

        # Step 4: Pull latest changes
        print("Pulling latest changes...", file=sys.stderr)
        cmd = ['git', '-C', str(nova_dir), 'pull']
        result = subprocess.run(cmd)

        if result.returncode != 0:
            print("Error: Failed to pull latest changes", file=sys.stderr)
            return 1

        # Step 5: Build master
        print("Building master...", file=sys.stderr)
        cmd = [
            'bash', '-ic', # use bash to get the alias
            f'ws-build -o {output_path} {" ".join(args.extra_args)}'
        ]

        print(f"Running: {' '.join(cmd)}", file=sys.stderr)

        try:
            result = subprocess.run(cmd)
            if result.returncode != 0:
                print("Error: Build failed", file=sys.stderr)
                return result.returncode
        except FileNotFoundError:
            print("Error: nom-build not found in PATH", file=sys.stderr)
            return 1

        print("\nSuccessfully updated and built master!", file=sys.stderr)
        return 0
