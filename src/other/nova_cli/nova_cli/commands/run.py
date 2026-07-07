"""Run command implementation"""
import subprocess
import sys
from pathlib import Path

from nova_cli.commands.base import Command
from nova_cli.completion import complete_packages, complete_executables


class RunCommand(Command):
    """Implements 'nova run' command"""

    @staticmethod
    def add_parser(subparsers):
        """Add run subcommand parser"""
        parser = subparsers.add_parser(
            'run',
            help='Run a ROS2 node',
            description='Wrapper for ros2 run with automatic .py extension'
        )

        package_arg = parser.add_argument(
            'package',
            help='Package name (e.g., science, drive)'
        )
        package_arg.completer = complete_packages

        node_arg = parser.add_argument(
            'node',
            help='Node/executable name (e.g., kiln). Will append .py if needed'
        )
        node_arg.completer = complete_executables

        return parser

    @staticmethod
    def execute(args):
        """Execute run command"""
        package = args.package

        # Check if package exists first
        if not RunCommand._package_exists(args.build_path, package):
            available = RunCommand._list_packages(args.build_path)
            print(f"Error: Package '{package}' not found.", file=sys.stderr)

            from nova_cli.utils import fuzzy_match
            suggestions = fuzzy_match(package, available)
            if suggestions:
                print(f"Did you mean: {', '.join(suggestions)}?", file=sys.stderr)

            return 1

        # Get available executables for this package
        executables = RunCommand._list_executables(args.build_path, package)

        # Try to find matching executable
        # 1. Try exact match
        if args.node in executables:
            node = args.node
        # 2. Try with .py extension
        elif f"{args.node}.py" in executables:
            node = f"{args.node}.py"
        # 3. Node not found
        else:
            print(f"Error: Executable '{args.node}' not found in package '{package}'.", file=sys.stderr)

            if executables:
                from nova_cli.utils import fuzzy_match
                suggestions = fuzzy_match(args.node, executables)
                if suggestions:
                    print(f"Did you mean: {', '.join(suggestions)}?", file=sys.stderr)
                else:
                    print(f"Available executables: {', '.join(executables[:10])}", file=sys.stderr)
                    if len(executables) > 10:
                        print(f"... and {len(executables) - 10} more", file=sys.stderr)

            return 1

        # Build ros2 command
        ros2_args = ['run', package, node] + args.extra_args

        # Execute the command
        return Command.run_ros2_command(args.build_path, ros2_args)

    @staticmethod
    def _package_exists(build_path, package_name):
        """Check if a package exists"""
        ros2_bin = build_path / "bin" / "ros2"
        result = subprocess.run(
            [str(ros2_bin), 'pkg', 'list'],
            capture_output=True,
            text=True
        )

        if result.returncode != 0:
            return False

        packages = result.stdout.strip().split('\n')
        return package_name in packages

    @staticmethod
    def _list_packages(build_path):
        """List all packages"""
        ros2_bin = build_path / "bin" / "ros2"
        result = subprocess.run(
            [str(ros2_bin), 'pkg', 'list'],
            capture_output=True,
            text=True
        )

        if result.returncode != 0:
            return []

        return result.stdout.strip().split('\n')

    @staticmethod
    def _list_executables(build_path, package_name):
        """List executables for a package"""
        ros2_bin = build_path / "bin" / "ros2"
        result = subprocess.run(
            [str(ros2_bin), 'pkg', 'executables', package_name],
            capture_output=True,
            text=True
        )

        if result.returncode != 0:
            return []

        # Output format: "package_name executable_name"
        executables = []
        for line in result.stdout.strip().split('\n'):
            if ' ' in line:
                _, executable = line.split(' ', 1)
                executables.append(executable)

        return executables
