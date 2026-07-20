"""Shared fixtures for nova_cli tests"""
import pytest
from pathlib import Path
from unittest.mock import MagicMock, patch
from types import SimpleNamespace

from nova_cli.commands import COMMANDS


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

    Works with any command registered in COMMANDS that runs subprocesses.
    Captures both ROS2 commands (via ros2_utils.run_ros2_command) and direct
    subprocess calls (via subprocess.run).

    Usage:
        cmd = nova_cli(["launch", "science", "urc"])
        cmd = nova_cli(["run", "science", "kiln"])
        cmd = nova_cli(["start", "run-gui"])
        cmd = nova_cli(["build", "master"])

    Override available packages/executables/scripts:
        cmd = nova_cli(["launch", "cameras"], packages=["cameras"])
        cmd = nova_cli(["start", "run-custom"], scripts=["run-custom"])

    Adding a new command:
        1. Register it in nova_cli/commands/__init__.py
        2. Tests automatically work - no conftest changes needed
    """
    from nova_cli.main import create_parser
    from nova_cli import ros2_utils, build_utils

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

    # Default scripts for start command
    DEFAULT_SCRIPTS = ["run-gui", "run-drive", "run-auto", "run-science"]

    def _cli(cli_args, *, packages=None, executables=None, scripts=None):
        pkg_list = packages if packages is not None else DEFAULT_PACKAGES
        exec_list = executables if executables is not None else DEFAULT_EXECUTABLES
        script_list = scripts if scripts is not None else DEFAULT_SCRIPTS

        # Parse the args
        parser = create_parser()
        args, extra_args = parser.parse_known_args(cli_args)
        args.extra_args = extra_args

        # Capture the command that would be executed
        captured_cmd = None

        def capture_ros2_cmd(bp, ros2_args):
            nonlocal captured_cmd
            ros2_bin = bp / "bin" / "ros2"
            captured_cmd = [str(ros2_bin)] + ros2_args
            return 0

        def capture_subprocess(cmd, **kwargs):
            nonlocal captured_cmd
            captured_cmd = list(cmd) if not isinstance(cmd, list) else cmd
            return MagicMock(returncode=0)

        def mock_pkg_exists(bp, name):
            return name in pkg_list

        def mock_list_executables(bp, pkg):
            return exec_list.get(pkg, [])

        def mock_list_packages(bp):
            return pkg_list

        def mock_validate_build_path(build_name):
            # Return a predictable build path for any build name
            return Path(f"{BUILD_ROOT}/{build_name}")

        # Mock for Path.exists() - checks if path is a launch dir or script
        def mock_path_exists(self):
            path_str = str(self)
            # Launch directories always exist
            if path_str.endswith("/launch"):
                return True
            # Check if it's a script in our list
            if "/launch/" in path_str:
                script_name = self.name
                return script_name in script_list
            # Default to True for other paths (like build dirs)
            return True

        # Mock for Path.is_file() - scripts are files
        def mock_path_is_file(self):
            path_str = str(self)
            if "/launch/" in path_str:
                script_name = self.name
                return script_name in script_list
            return False

        with patch.object(ros2_utils, 'run_ros2_command', side_effect=capture_ros2_cmd):
            with patch.object(ros2_utils, 'package_exists', side_effect=mock_pkg_exists):
                with patch.object(ros2_utils, 'list_executables', side_effect=mock_list_executables):
                    with patch.object(ros2_utils, 'list_packages', side_effect=mock_list_packages):
                        with patch.object(build_utils, 'validate_build_path', side_effect=mock_validate_build_path):
                            with patch('subprocess.run', side_effect=capture_subprocess):
                                with patch.object(Path, 'exists', mock_path_exists):
                                    with patch.object(Path, 'is_file', mock_path_is_file):
                                        with patch.object(Path, 'home', return_value=Path('/home/test')):
                                            # Generic command dispatch - works for ANY command
                                            cmd_class = COMMANDS[args.command]
                                            cmd_class.execute(args)

        return captured_cmd

    return _cli
