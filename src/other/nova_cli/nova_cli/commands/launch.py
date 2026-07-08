"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Launches ROS2 launch files with automatic package and file name
transformations. Appends _bringup to package names and .launch.py
to launch file names automatically. Includes autocomplete.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
EXAMPLES:
  nova launch drive             # Launches drive_bringup/drive.launch.py
  nova launch arm control       # Launches arm_bringup/control.launch.py
  nova launch teleop drive      # Launches teleop_drive/teleop.launch.py
  nova launch science sim:=true # Pass ROS2 arguments
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        nova_cli
AUTHOR(S):      Felicity Matthews
CREATION:       06/07/2026
EDITED:         09/07/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import sys

from nova_cli.commands.base import Command
from nova_cli import ros2_utils


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

        package_arg = parser.add_argument(
            'package',
            help='Package name (e.g., science, auto, arm). Will append _bringup automatically'
        )
        package_arg.completer = LaunchCommand.complete_package

        launch_file_arg = parser.add_argument(
            'launch_file',
            nargs='?',
            default=None,
            help='Launch file name (e.g., urc, drive). Defaults to package name if omitted. Will append .launch.py automatically'
        )
        launch_file_arg.completer = LaunchCommand.complete_launch_file

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
        if not ros2_utils.package_exists(args.build_path, resolved_package):
            available = ros2_utils.list_packages(args.build_path)
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
        return ros2_utils.run_ros2_command(args.build_path, ros2_args)

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
            if ros2_utils.package_exists(build_path, bringup_pkg):
                return bringup_pkg

        # 2. If not found, try exact match
        if ros2_utils.package_exists(build_path, package):
            return package

        # 3. Not found - return original for error handling
        return package

    @staticmethod
    def complete_package(prefix, parsed_args, **kwargs):
        """Complete package names for launch command."""
        build_path = ros2_utils.get_build_path()
        packages = ros2_utils.list_packages(build_path)

        results = []
        for pkg in packages:
            if pkg.startswith(prefix):
                results.append(pkg)
            # Also suggest base names for _bringup packages
            if pkg.endswith('_bringup'):
                base = pkg[:-8]
                if base.startswith(prefix):
                    results.append(base)
        return results

    @staticmethod
    def complete_launch_file(prefix, parsed_args, **kwargs):
        """Complete launch file names."""
        if not hasattr(parsed_args, 'package') or not parsed_args.package:
            return []

        package = parsed_args.package
        if package == "teleop":
            subsystems = ['drive', 'arm', 'science', 'ec']
            return [s for s in subsystems if s.startswith(prefix)]

        build_path = ros2_utils.get_build_path()

        # Try _bringup suffix first
        test_pkg = package if package.endswith('_bringup') else f"{package}_bringup"
        launch_files = ros2_utils.list_launch_files(build_path, test_pkg)

        if not launch_files:
            # Try without _bringup
            launch_files = ros2_utils.list_launch_files(build_path, package)

        return [f for f in launch_files if f.startswith(prefix)]
