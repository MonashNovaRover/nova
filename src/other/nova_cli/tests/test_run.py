"""Tests for run command"""
import pytest
from unittest.mock import patch

from nova_cli.commands.run import RunCommand
from nova_cli import ros2_utils


class TestRunCLI:
    """Full CLI tests for nova run command."""

    def test_cpp_node(self, nova_cli):
        # C++ nodes have no extension
        cmd = nova_cli(["run", "science", "controller"])
        assert cmd == ["/builds/active/bin/ros2", "run", "science", "controller"]

    def test_python_node_explicit(self, nova_cli):
        # Python node with explicit .py
        cmd = nova_cli(["run", "science", "kiln.py"])
        assert cmd == ["/builds/active/bin/ros2", "run", "science", "kiln.py"]

    def test_python_node_auto_suffix(self, nova_cli):
        # User types "kiln", only "kiln.py" exists -> auto-adds .py
        cmd = nova_cli(["run", "science", "kiln"])
        assert cmd == ["/builds/active/bin/ros2", "run", "science", "kiln.py"]

    def test_extra_args(self, nova_cli):
        cmd = nova_cli(["run", "science", "controller", "--ros-args", "-p", "param:=value"])
        assert cmd == ["/builds/active/bin/ros2", "run", "science", "controller", "--ros-args", "-p", "param:=value"]

    def test_build_flag_short(self, nova_cli):
        cmd = nova_cli(["run", "science", "controller", "-b", "main"])
        assert cmd == ["/builds/main/bin/ros2", "run", "science", "controller"]

    def test_build_flag_long(self, nova_cli):
        cmd = nova_cli(["run", "science", "controller", "--build", "auto"])
        assert cmd == ["/builds/auto/bin/ros2", "run", "science", "controller"]

    def test_build_flag_after_command(self, nova_cli):
        cmd = nova_cli(["run", "-b", "arm", "science", "controller"])
        assert cmd == ["/builds/arm/bin/ros2", "run", "science", "controller"]

    def test_build_flag_with_extra_args(self, nova_cli):
        cmd = nova_cli(["run", "science", "controller", "--ros-args", "-b", "main"])
        assert cmd == ["/builds/main/bin/ros2", "run", "science", "controller", "--ros-args"]


class TestExecutableNotFound:
    """Tests for executable not found handling."""

    def test_returns_error(self, mock_args, capsys):
        mock_args.package = "science"
        mock_args.node = "nonexistent"

        with patch.object(ros2_utils, 'package_exists', return_value=True):
            with patch.object(ros2_utils, 'list_executables', return_value=['kiln', 'other']):
                result = RunCommand.execute(mock_args)
                assert result == 1
                assert "Executable 'nonexistent' not found" in capsys.readouterr().err


class TestPackageNotFound:
    """Tests for package not found handling."""

    def test_returns_error(self, mock_args, capsys):
        mock_args.package = "nonexistent"
        mock_args.node = "some_node"

        with patch.object(ros2_utils, 'package_exists', return_value=False):
            with patch.object(ros2_utils, 'list_packages', return_value=['science', 'drive']):
                result = RunCommand.execute(mock_args)
                assert result == 1
                assert "Package 'nonexistent' not found" in capsys.readouterr().err
