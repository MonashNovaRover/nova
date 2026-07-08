"""Shared fixtures for nova_cli tests"""
import pytest
from pathlib import Path
from unittest.mock import MagicMock, patch
from types import SimpleNamespace

from nova_cli.commands.launch import LaunchCommand
from nova_cli.commands.run import RunCommand


# Predictable build path prefix for assertions
BUILD_ROOT = "/builds"


@pytest.fixture
def mock_build_path(tmp_path):
    """Create a mock build directory structure"""
    build_dir = tmp_path / "Builds" / "active"
    bin_dir = build_dir / "bin"
    bin_dir.mkdir(parents=True)
    (bin_dir / "ros2").touch()
    return build_dir


@pytest.fixture
def mock_args(mock_build_path):
    """Create a mock args namespace for command execution"""
    return SimpleNamespace(
        build_path=mock_build_path,
        extra_args=[],
    )


@pytest.fixture
def nova_cli():
    """
    Simulate the full nova CLI and return the command that would be executed.

    Returns the full command list: ["/builds/<build>/bin/ros2", "launch", ...]

    Usage:
        cmd = nova_cli(["launch", "science", "urc"])
        assert cmd == ["/builds/active/bin/ros2", "launch", "science_bringup", "urc.launch.py"]

        # Override available packages:
        cmd = nova_cli(["launch", "cameras"], packages=["cameras"])
        assert cmd == ["/builds/active/bin/ros2", "launch", "cameras", "cameras.launch.py"]
    """
    from nova_cli.main import extract_global_flags, create_parser
    from nova_cli import ros2_utils

    # Default packages that "exist" - covers most test cases
    DEFAULT_PACKAGES = [
        "science_bringup", "science",
        "auto_bringup", "auto",
        "drive_bringup", "drive",
        "arm_bringup", "arm",
        "teleop_bringup", "teleop",
        "teleop_drive_bringup",
        "teleop_arm_bringup",
        "cameras_bringup", "cameras",
    ]

    # All executables that "exist" per package
    # C++ nodes: no extension (e.g., controller, planner)
    # Python nodes: .py extension (e.g., kiln.py, sensor.py)
    DEFAULT_EXECUTABLES = {
        "science": [
            "controller",     # C++ node
            "kiln.py",        # Python node
            "sensor.py",      # Python node (tests .py auto-add when user types "sensor")
        ],
        "auto": ["planner", "navigator"],  # C++ nodes
        "drive": ["controller"],            # C++ node
    }

    def _cli(cli_args, *, packages=None, executables=None):
        pkg_list = packages if packages is not None else DEFAULT_PACKAGES
        exec_list = executables if executables is not None else DEFAULT_EXECUTABLES

        # Extract global flags (like -b)
        global_flags, filtered_argv = extract_global_flags(cli_args)

        # Parse the filtered args
        parser = create_parser()
        args, extra_args = parser.parse_known_args(filtered_argv)

        # Apply global flags
        for key, value in global_flags.items():
            setattr(args, key, value)

        # Use predictable build path
        build_name = args.build
        build_path = Path(f"{BUILD_ROOT}/{build_name}")
        args.build_path = build_path
        args.extra_args = extra_args

        # Capture the command
        captured_cmd = None

        def capture_ros2_cmd(bp, ros2_args):
            nonlocal captured_cmd
            ros2_bin = bp / "bin" / "ros2"
            captured_cmd = [str(ros2_bin)] + ros2_args
            return 0

        def mock_pkg_exists(bp, name):
            return name in pkg_list

        def mock_list_executables(bp, pkg):
            return exec_list.get(pkg, [])

        def mock_list_packages(bp):
            return pkg_list

        with patch.object(ros2_utils, 'run_ros2_command', side_effect=capture_ros2_cmd):
            with patch.object(ros2_utils, 'package_exists', side_effect=mock_pkg_exists):
                with patch.object(ros2_utils, 'list_executables', side_effect=mock_list_executables):
                    with patch.object(ros2_utils, 'list_packages', side_effect=mock_list_packages):
                        if args.command == 'launch':
                            LaunchCommand.execute(args)
                        elif args.command == 'run':
                            RunCommand.execute(args)

        return captured_cmd

    return _cli
