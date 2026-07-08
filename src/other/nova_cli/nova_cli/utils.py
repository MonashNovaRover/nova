"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Utility functions for nova CLI. Includes build path validation,
listing available builds, and fuzzy string matching for suggestions.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        nova_cli
AUTHOR(S):      Felicity Matthews
CREATION:       06/07/2026
EDITED:         09/07/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import os
from pathlib import Path
from difflib import get_close_matches


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


def fuzzy_match(target, options, n=3, cutoff=0.6):
    """
    Find close matches to target string in options.

    Args:
        target: String to match
        options: List of possible matches
        n: Max number of suggestions
        cutoff: Similarity threshold (0-1)

    Returns:
        List of close matches
    """
    return get_close_matches(target, options, n=n, cutoff=cutoff)
