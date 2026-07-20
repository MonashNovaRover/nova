"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Runs ROS2 node executables with automatic .py extension handling.
Validates package and executable existence with fuzzy matching
suggestions on error. Includes autocomplete.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
EXAMPLES:
  nova run science kiln         # Runs science/kiln.py
  nova run drive motor          # Runs drive/motor.py
  nova run arm controller       # Runs arm/controller.py
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        nova_cli
AUTHOR(S):      Felicity Matthews
CREATION:       06/07/2026
EDITED:         09/07/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import sys

from nova_cli.commands.base import Command
from nova_cli.build_utils import add_build_argument, validate_build_arg
from nova_cli import ros2_utils


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

        add_build_argument(parser)

        package_arg = parser.add_argument(
            'package',
            help='Package name (e.g., science, drive)'
        )
        package_arg.completer = RunCommand.complete_package

        node_arg = parser.add_argument(
            'node',
            help='Node/executable name (e.g., kiln). Will append .py if needed'
        )
        node_arg.completer = RunCommand.complete_executable

        return parser

    @staticmethod
    def execute(args):
        if (err := validate_build_arg(args)) is not None:
            return err

        """Execute run command"""
        package = args.package

        # Check if package exists first
        if not ros2_utils.package_exists(args.build_path, package):
            available = ros2_utils.list_packages(args.build_path)
            print(f"Error: Package '{package}' not found.", file=sys.stderr)

            from nova_cli.utils import fuzzy_match
            suggestions = fuzzy_match(package, available)
            if suggestions:
                print(f"Did you mean: {', '.join(suggestions)}?", file=sys.stderr)

            return 1

        # Get available executables for this package
        executables = ros2_utils.list_executables(args.build_path, package)

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
        return ros2_utils.run_ros2_command(args.build_path, ros2_args)
    
    @staticmethod
    def complete_package(prefix, parsed_args, **kwargs):
        """Complete package names."""
        build_path = ros2_utils.get_build_path()
        packages = ros2_utils.list_packages(build_path)
        return [p for p in packages if p.startswith(prefix)]

    @staticmethod
    def complete_executable(prefix, parsed_args, **kwargs):
        """Complete executable names."""
        if not hasattr(parsed_args, 'package') or not parsed_args.package:
            return []
        build_path = ros2_utils.get_build_path()
        executables = ros2_utils.list_executables(build_path, parsed_args.package)
        return [e for e in executables if e.startswith(prefix)]
