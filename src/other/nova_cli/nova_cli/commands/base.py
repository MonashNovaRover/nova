"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Abstract base class for nova CLI commands. Provides common interface
for parser registration and command execution, plus shared utilities
for running ros2 commands.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        nova_cli
AUTHOR(S):      Felicity Matthews
CREATION:       26/01/26
EDITED:         26/07/09
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import subprocess
import sys
from abc import ABC, abstractmethod


class Command(ABC):
    """Base class for nova commands"""

    @staticmethod
    @abstractmethod
    def add_parser(subparsers):
        """Add this command to the argument parser"""
        pass

    @staticmethod
    @abstractmethod
    def execute(args):
        """Execute the command"""
        pass

    @staticmethod
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
