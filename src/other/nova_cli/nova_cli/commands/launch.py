"""Launch command implementation"""
import subprocess
import sys
from pathlib import Path

from nova_cli.commands.base import Command


class LaunchCommand(Command):
    """Implements 'nova launch' command"""

    @staticmethod
    def add_parser(subparsers):
        """Add launch subcommand parser"""
        parser = subparsers.add_parser(
            'launch',
            help='Launch a ROS2 launch file',
            description='Wrapper for ros2 launch with automatic transformations'
        )

        parser.add_argument(
            'package',
            help='Package name (e.g., science, auto, arm). Will append _bringup automatically'
        )

        parser.add_argument(
            'launch_file',
            nargs='?',
            default=None,
            help='Launch file name (e.g., urc, drive). Defaults to package name if omitted. Will append .launch.py automatically'
        )

        # Note: extra args are captured in parse_known_args()

        return parser

    @staticmethod
    def execute(args):
        """Execute launch command"""
        package = args.package
        launch_file = args.launch_file
        extra_args = list(args.extra_args)

        # If launch_file looks like a ROS2 arg, treat it as extra_args
        if launch_file and ':=' in launch_file:
            extra_args = [launch_file] + extra_args
            launch_file = None

        # Default launch_file to package name if not provided
        if launch_file is None:
            launch_file = package

        # Special case: "launch teleop <subsystem>"
        # Example: "launch teleop drive" → package="teleop_drive", file="teleop"
        if package == "teleop" and launch_file and launch_file != "teleop":
            package = f"teleop_{launch_file}"
            launch_file = "teleop"

        # Resolve package name with smart _bringup handling
        resolved_package = LaunchCommand._resolve_package_name(args.build_path, package)

        # Check if package exists
        if not LaunchCommand._package_exists(args.build_path, resolved_package):
            available = LaunchCommand._list_bringup_packages(args.build_path)
            print(f"Error: Package '{resolved_package}' not found.", file=sys.stderr)

            # Suggest alternatives
            from nova_cli.utils import fuzzy_match
            suggestions = fuzzy_match(resolved_package, available)
            if suggestions:
                print(f"Did you mean: {', '.join(suggestions)}?", file=sys.stderr)
            else:
                print(f"Available packages: {', '.join(available[:10])}", file=sys.stderr)
                if len(available) > 10:
                    print(f"... and {len(available) - 10} more", file=sys.stderr)

            return 1

        # Transform launch file name
        if not launch_file.endswith('.launch.py'):
            launch_file = f"{launch_file}.launch.py"

        # Build ros2 command
        ros2_args = ['launch', resolved_package, launch_file] + extra_args

        # Execute the command
        return Command.run_ros2_command(args.build_path, ros2_args)

    @staticmethod
    def _resolve_package_name(build_path, package):
        """
        Resolve package name with smart _bringup handling.

        Tries with _bringup appended first (default behavior), then falls back
        to exact match if not found.

        Args:
            build_path: Path to the build directory
            package: Package name to resolve

        Returns:
            Resolved package name
        """
        # 1. Try with _bringup appended first (default behavior)
        if not package.endswith('_bringup'):
            bringup_pkg = f"{package}_bringup"
            if LaunchCommand._package_exists(build_path, bringup_pkg):
                return bringup_pkg

        # 2. If not found, try exact match
        if LaunchCommand._package_exists(build_path, package):
            return package

        # 3. Not found - return original for error handling
        return package

    @staticmethod
    def _package_exists(build_path, package_name):
        """Check if a package exists in the build"""
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
    def _list_bringup_packages(build_path):
        """List all packages (prioritize *_bringup packages)"""
        ros2_bin = build_path / "bin" / "ros2"
        result = subprocess.run(
            [str(ros2_bin), 'pkg', 'list'],
            capture_output=True,
            text=True
        )

        if result.returncode != 0:
            return []

        packages = result.stdout.strip().split('\n')
        # Return all packages, but _bringup packages are more relevant
        return sorted(packages)
