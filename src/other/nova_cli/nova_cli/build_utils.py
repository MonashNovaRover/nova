"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Build-related utilities for nova CLI. Includes build path validation,
listing available builds, and argument helpers for commands that need
build selection.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        nova_cli
AUTHOR(S):      Felicity Matthews
CREATION:       09/07/2026
EDITED:         09/07/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import sys
from pathlib import Path


BUILDS_DIR = Path.home() / "Builds"


def validate_build_path(build_name):
    """
    Validate that a build exists and return its path.

    Args:
        build_name: Name of the build (e.g., 'active', 'master', 'auto')

    Returns:
        Path to build directory if valid, None otherwise
    """
    build_path = BUILDS_DIR / build_name

    # Check if build directory exists
    if not build_path.exists():
        return None

    # Check if ros2 executable exists
    ros2_bin = build_path / "bin" / "ros2"
    if not ros2_bin.exists():
        return None

    return build_path


def list_available_builds():
    """
    List all available builds in ~/Builds/

    Returns:
        List of build names
    """
    if not BUILDS_DIR.exists():
        return []

    builds = []
    for item in BUILDS_DIR.iterdir():
        if item.is_dir() or item.is_symlink():
            # Check if it has a bin/ros2
            if (item / "bin" / "ros2").exists():
                builds.append(item.name)

    return sorted(builds)


def get_ros2_executable(build_path):
    """Get the path to ros2 executable for a build"""
    return build_path / "bin" / "ros2"


def add_build_argument(parser):
    """Add -b/--build argument to a command parser."""
    parser.add_argument(
        '-b', '--build',
        default='active',
        help='Build to use (default: active). Available: master, auto, arm, drive, etc.'
    )


def validate_build_arg(args):
    """Validate args.build and set args.build_path. Returns error code or None."""
    # Skip validation if build_path is already set (e.g., in tests)
    if hasattr(args, 'build_path') and args.build_path is not None:
        return None

    build_path = validate_build_path(args.build)
    if not build_path:
        available = list_available_builds()
        print(f"Error: Build '{args.build}' not found.", file=sys.stderr)
        if available:
            print(f"Available builds: {', '.join(available)}", file=sys.stderr)
        else:
            print(f"No builds found in ~/Builds/", file=sys.stderr)
        return 1
    args.build_path = build_path
    return None
