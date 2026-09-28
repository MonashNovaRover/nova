"""Tests for start command"""
import pytest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch, MagicMock

from nova_cli.commands.start import StartCommand


@pytest.fixture
def start_scripts_dir(tmp_path):
    """Create a mock launch directory with scripts"""
    launch_dir = tmp_path / "launch"
    launch_dir.mkdir()

    # Create some mock scripts
    for script in ["run-gui", "run-drive", "run-auto", "run-science"]:
        script_path = launch_dir / script
        script_path.write_text("#!/bin/bash\necho test")
        script_path.chmod(0o755)

    return tmp_path


class TestStartCLI:
    """Full CLI tests for nova start command."""

    def test_basic_start(self, nova_cli):
        cmd = nova_cli(["start", "run-gui"])
        assert cmd == ["/builds/active/launch/run-gui"]

    def test_start_with_args(self, nova_cli):
        cmd = nova_cli(["start", "run-auto", "nova@10.0.0.2"])
        assert cmd == ["/builds/active/launch/run-auto", "nova@10.0.0.2"]

    def test_start_with_build_flag(self, nova_cli):
        cmd = nova_cli(["start", "run-gui", "-b", "main"])
        assert cmd == ["/builds/main/launch/run-gui"]

    def test_start_build_flag_position(self, nova_cli):
        cmd = nova_cli(["start", "-b", "arm", "run-drive"])
        assert cmd == ["/builds/arm/launch/run-drive"]


class TestStartCommand:
    """Unit tests for StartCommand error handling"""

    def test_execute_script_not_found(self, start_scripts_dir):
        """Test error when script doesn't exist"""
        args = SimpleNamespace(
            build_path=start_scripts_dir,
            script="nonexistent",
            extra_args=[],
        )

        result = StartCommand.execute(args)
        assert result == 1

    def test_execute_launch_dir_not_found(self, tmp_path):
        """Test error when launch directory doesn't exist"""
        args = SimpleNamespace(
            build_path=tmp_path,
            script="run-gui",
            extra_args=[],
        )

        result = StartCommand.execute(args)
        assert result == 1

    def test_execute_runs_script(self, start_scripts_dir):
        """Test that execute runs the script"""
        args = SimpleNamespace(
            build_path=start_scripts_dir,
            script="run-gui",
            extra_args=[],
        )

        mock_result = MagicMock()
        mock_result.returncode = 0

        with patch('subprocess.run', return_value=mock_result) as mock_run:
            result = StartCommand.execute(args)
            assert result == 0
            mock_run.assert_called_once()
            cmd = mock_run.call_args[0][0]
            assert cmd[0] == str(start_scripts_dir / "launch" / "run-gui")

    def test_execute_with_extra_args(self, start_scripts_dir):
        """Test that extra args are passed to script"""
        args = SimpleNamespace(
            build_path=start_scripts_dir,
            script="run-auto",
            extra_args=["nova@10.0.0.2", "nova@10.0.0.3"],
        )

        mock_result = MagicMock()
        mock_result.returncode = 0

        with patch('subprocess.run', return_value=mock_result) as mock_run:
            result = StartCommand.execute(args)
            assert result == 0
            cmd = mock_run.call_args[0][0]
            assert cmd[0] == str(start_scripts_dir / "launch" / "run-auto")
            assert cmd[1] == "nova@10.0.0.2"
            assert cmd[2] == "nova@10.0.0.3"


class TestCompleteScripts:
    """Tests for StartCommand.complete_script"""

    def test_complete_scripts(self, start_scripts_dir):
        """Test script completion"""
        with patch('nova_cli.ros2_utils.get_build_path', return_value=start_scripts_dir):
            scripts = StartCommand.complete_script("run-", None)
            assert "run-gui" in scripts
            assert "run-drive" in scripts

    def test_complete_scripts_prefix_filter(self, start_scripts_dir):
        """Test script completion filters by prefix"""
        with patch('nova_cli.ros2_utils.get_build_path', return_value=start_scripts_dir):
            scripts = StartCommand.complete_script("run-g", None)
            assert "run-gui" in scripts
            assert "run-drive" not in scripts

    def test_complete_scripts_no_launch_dir(self, tmp_path):
        """Test completion when launch dir doesn't exist"""
        with patch('nova_cli.ros2_utils.get_build_path', return_value=tmp_path):
            scripts = StartCommand.complete_script("", None)
            assert scripts == []
