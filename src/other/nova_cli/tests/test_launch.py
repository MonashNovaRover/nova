"""Tests for launch command"""
from unittest.mock import patch, MagicMock

from nova_cli.commands.launch import LaunchCommand
from nova_cli import ros2_utils


class TestLaunchCLI:
    """Full CLI tests for nova launch command."""

    def test_basic_launch(self, nova_cli):
        cmd = nova_cli(["launch", "science", "urc"])
        assert cmd == ["/builds/active/bin/ros2", "launch", "science_bringup", "urc.launch.py"]

    def test_default_launch_file(self, nova_cli):
        cmd = nova_cli(["launch", "drive"])
        assert cmd == ["/builds/active/bin/ros2", "launch", "drive_bringup", "drive.launch.py"]

    def test_launch_file_with_suffix(self, nova_cli):
        cmd = nova_cli(["launch", "science", "urc.launch.py"])
        assert cmd == ["/builds/active/bin/ros2", "launch", "science_bringup", "urc.launch.py"]

    def test_teleop_drive_shorthand(self, nova_cli):
        cmd = nova_cli(["launch", "teleop", "drive"])
        assert cmd == ["/builds/active/bin/ros2", "launch", "teleop_drive_bringup", "teleop.launch.py"]

    def test_teleop_arm_shorthand(self, nova_cli):
        cmd = nova_cli(["launch", "teleop", "arm"])
        assert cmd == ["/builds/active/bin/ros2", "launch", "teleop_arm_bringup", "teleop.launch.py"]

    def test_teleop_no_subsystem(self, nova_cli):
        cmd = nova_cli(["launch", "teleop"])
        assert cmd == ["/builds/active/bin/ros2", "launch", "teleop_bringup", "teleop.launch.py"]

    def test_extra_args(self, nova_cli):
        cmd = nova_cli(["launch", "auto", "sim", "sim:=True", "debug:=False"])
        assert cmd == ["/builds/active/bin/ros2", "launch", "auto_bringup", "sim.launch.py", "sim:=True", "debug:=False"]

    def test_build_flag_short(self, nova_cli):
        cmd = nova_cli(["launch", "science", "urc", "-b", "master"])
        assert cmd == ["/builds/master/bin/ros2", "launch", "science_bringup", "urc.launch.py"]

    def test_build_flag_long(self, nova_cli):
        cmd = nova_cli(["launch", "science", "urc", "--build", "auto"])
        assert cmd == ["/builds/auto/bin/ros2", "launch", "science_bringup", "urc.launch.py"]

    def test_build_flag_after_command(self, nova_cli):
        cmd = nova_cli(["launch", "-b", "arm", "science", "urc"])
        assert cmd == ["/builds/arm/bin/ros2", "launch", "science_bringup", "urc.launch.py"]

    def test_build_flag_with_extra_args(self, nova_cli):
        cmd = nova_cli(["launch", "auto", "sim", "sim:=True", "-b", "master"])
        assert cmd == ["/builds/master/bin/ros2", "launch", "auto_bringup", "sim.launch.py", "sim:=True"]

    def test_cameras_fallback_no_bringup(self, nova_cli):
        cmd = nova_cli(["launch", "cameras"], packages=["cameras"])
        assert cmd == ["/builds/active/bin/ros2", "launch", "cameras", "cameras.launch.py"]

    def test_drive_with_build_flag(self, nova_cli):
        cmd = nova_cli(["launch", "drive", "-b", "master"])
        assert cmd == ["/builds/master/bin/ros2", "launch", "drive_bringup", "drive.launch.py"]

    def test_drive_with_extra_arg(self, nova_cli):
        cmd = nova_cli(["launch", "drive", "auto:=true"])
        assert cmd == ["/builds/active/bin/ros2", "launch", "drive_bringup", "drive.launch.py", "auto:=true"]


class TestResolvePackageName:
    """Tests for _resolve_package_name."""

    def test_appends_bringup_when_exists(self, mock_build_path):
        with patch.object(ros2_utils, 'package_exists') as mock_exists:
            mock_exists.side_effect = lambda bp, name: name == 'science_bringup'
            result = LaunchCommand._resolve_package_name(mock_build_path, 'science')
            assert result == 'science_bringup'

    def test_fallback_to_exact_name(self, mock_build_path):
        with patch.object(ros2_utils, 'package_exists') as mock_exists:
            mock_exists.side_effect = lambda bp, name: name == 'science'
            result = LaunchCommand._resolve_package_name(mock_build_path, 'science')
            assert result == 'science'

    def test_already_has_bringup_suffix(self, mock_build_path):
        with patch.object(ros2_utils, 'package_exists', return_value=True):
            result = LaunchCommand._resolve_package_name(mock_build_path, 'science_bringup')
            assert result == 'science_bringup'
