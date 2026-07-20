"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Shared utilities for ROS2 package and executable discovery.
Uses filesystem access for reliable operation during autocomplete.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        nova_cli
AUTHOR(S):      Felicity Matthews
CREATION:       09/07/2026
EDITED:         09/07/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import os
import subprocess
import sys
from pathlib import Path


def get_build_path():
    """Get the build path from environment or default to active."""
    return Path.home() / "Builds" / "active"


def list_packages(build_path: Path) -> list[str]:
    """List all ROS2 packages by reading from the filesystem.

    Reads directly from build_path/share/ instead of calling 'ros2 pkg list'
    to avoid environment setup issues during autocomplete.
    """
    share_dir = build_path / "share"
    if not share_dir.exists():
        return []

    packages = []
    for item in share_dir.iterdir():
        if item.is_dir():
            packages.append(item.name)

    return sorted(packages)


def package_exists(build_path: Path, package_name: str) -> bool:
    """Check if a package exists."""
    return package_name in list_packages(build_path)


def list_executables(build_path: Path, package: str) -> list[str]:
    """List executables for a package by reading from the filesystem.

    Reads directly from build_path/lib/{package}/ instead of calling 'ros2 pkg executables'
    to avoid environment setup issues during autocomplete.
    """
    lib_dir = build_path / "lib" / package
    if not lib_dir.exists():
        return []

    executables = []
    for item in lib_dir.iterdir():
        if item.is_file() and os.access(item, os.X_OK):
            executables.append(item.name)

    return sorted(executables)


def list_launch_files(build_path: Path, package: str) -> list[str]:
    """List launch files for a package."""
    launch_dir = build_path / "share" / package / "launch"
    if not launch_dir.exists():
        return []
    return sorted(f.stem.replace('.launch', '') for f in launch_dir.glob("*.launch.py"))


def list_scripts(launch_dir: Path) -> list[str]:
    """List executable scripts in a directory."""
    if not launch_dir.exists():
        return []
    return sorted(f.name for f in launch_dir.iterdir()
                  if f.is_file() and os.access(f, os.X_OK))


def run_ros2_command(build_path, ros2_args):
    """
    Run a ros2 command and return exit code.

    Args:
        build_path: Path to the build directory
        ros2_args: List of arguments to pass to ros2

    Returns:
        Exit code from ros2 command
    """
    ros2_bin = build_path / "bin" / "ros2"
    cmd = [str(ros2_bin)] + ros2_args

    # Print the command being run (helpful for debugging)
    print(f"Running: {' '.join(cmd)}", file=sys.stderr)

    # Run the command, inheriting stdout/stderr
    result = subprocess.run(cmd)
    return result.returncode
