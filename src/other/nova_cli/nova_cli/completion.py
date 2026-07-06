"""Bash completion support for nova CLI"""
import os
import subprocess
from pathlib import Path


def _get_build_path():
    """Get the build path from environment or default to active"""
    # For completion, default to active build
    # TODO: Parse -b/--build from command line args if present
    return Path.home() / "Builds" / "active"


def _run_ros2_command(args):
    """Run a ros2 command and return output"""
    build_path = _get_build_path()
    ros2_bin = build_path / "bin" / "ros2"

    if not ros2_bin.exists():
        return ""

    try:
        result = subprocess.run(
            [str(ros2_bin)] + args,
            capture_output=True,
            text=True,
            timeout=5
        )
        return result.stdout.strip() if result.returncode == 0 else ""
    except:
        return ""


def complete_packages(prefix, parsed_args, **kwargs):
    """Complete package names"""
    output = _run_ros2_command(['pkg', 'list'])
    if not output:
        return []

    packages = output.split('\n')
    return [p for p in packages if p.startswith(prefix)]


def complete_packages_smart(prefix, parsed_args, **kwargs):
    """Complete package names for launch command (shows base names too)"""
    output = _run_ros2_command(['pkg', 'list'])
    if not output:
        return []

    packages = output.split('\n')

    # Return both full names and base names for _bringup packages
    results = []
    for pkg in packages:
        if pkg.startswith(prefix):
            results.append(pkg)
        # Also suggest without _bringup suffix
        if pkg.endswith('_bringup'):
            base = pkg[:-8]
            if base.startswith(prefix):
                results.append(base)

    return results


def complete_launch_files(prefix, parsed_args, **kwargs):
    """Complete launch file names in the selected package"""
    if not hasattr(parsed_args, 'package') or not parsed_args.package:
        return []

    # Handle teleop shorthand
    package = parsed_args.package
    if package == "teleop":
        # For teleop, we complete subsystem names
        # This would need a list of valid subsystems - for now return common ones
        subsystems = ['drive', 'arm', 'science', 'ec']
        return [s for s in subsystems if s.startswith(prefix)]

    # Try to resolve package name (try _bringup first)
    test_pkg = package if package.endswith('_bringup') else f"{package}_bringup"

    # Try to get launch files using ros2 pkg prefix
    try:
        build_path = _get_build_path()

        # Use ros2 to find package prefix
        cmd = [
            str(build_path / "bin" / "ros2"),
            "pkg",
            "prefix",
            test_pkg
        ]

        result = subprocess.run(cmd, capture_output=True, text=True, timeout=5)
        if result.returncode == 0:
            package_prefix = Path(result.stdout.strip())
            launch_dir = package_prefix / "share" / test_pkg / "launch"

            if launch_dir.exists():
                # List .launch.py files
                launch_files = []
                for f in launch_dir.glob("*.launch.py"):
                    # Return without .launch.py extension
                    base_name = f.stem.replace('.launch', '')
                    if base_name.startswith(prefix):
                        launch_files.append(base_name)

                return launch_files
    except:
        pass

    return []


def complete_executables(prefix, parsed_args, **kwargs):
    """Complete executable names in the selected package"""
    if not hasattr(parsed_args, 'package') or not parsed_args.package:
        return []

    package = parsed_args.package
    output = _run_ros2_command(['pkg', 'executables', package])

    if not output:
        return []

    # Parse executables (format: "package_name executable_name")
    executables = []
    for line in output.split('\n'):
        if ' ' in line:
            _, executable = line.split(' ', 1)
            # Return both with and without .py extension
            if executable.endswith('.py'):
                base_name = executable[:-3]
                if base_name.startswith(prefix) or executable.startswith(prefix):
                    executables.append(base_name)
            else:
                if executable.startswith(prefix):
                    executables.append(executable)

    return executables
