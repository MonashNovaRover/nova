"""Tests for start command"""
import pytest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch, MagicMock

from nova_cli.commands.start import StartCommand


@pytest.fixture
def mock_launch_dir(tmp_path):
    """Create a mock launch directory with scripts"""
    launch_dir = tmp_path / "launch"
    launch_dir.mkdir()

    # Create some mock scripts
    for script in ["run-gui", "run-drive", "run-auto", "run-science"]:
        script_path = launch_dir / script
        script_path.write_text("#!/bin/bash\necho test")
        script_path.chmod(0o755)

    return tmp_path


@pytest.fixture
def mock_args(mock_launch_dir):
    """Create a mock args namespace"""
    return SimpleNamespace(
        build_path=mock_launch_dir,
        script="run-gui",
        extra_args=[],
    )


class TestStartCommand:
    """Tests for StartCommand"""

    def test_execute_script_not_found(self, mock_launch_dir):
        """Test error when script doesn't exist"""
        args = SimpleNamespace(
            build_path=mock_launch_dir,
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

    def test_execute_runs_script(self, mock_launch_dir):
        """Test that execute runs the script"""
        args = SimpleNamespace(
            build_path=mock_launch_dir,
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
            assert cmd[0] == str(mock_launch_dir / "launch" / "run-gui")

    def test_execute_with_extra_args(self, mock_launch_dir):
        """Test that extra args are passed to script"""
        args = SimpleNamespace(
            build_path=mock_launch_dir,
            script="run-auto",
            extra_args=["nova@10.0.0.2", "nova@10.0.0.3"],
        )

        mock_result = MagicMock()
        mock_result.returncode = 0

        with patch('subprocess.run', return_value=mock_result) as mock_run:
            result = StartCommand.execute(args)
            assert result == 0
            cmd = mock_run.call_args[0][0]
            assert cmd[0] == str(mock_launch_dir / "launch" / "run-auto")
            assert cmd[1] == "nova@10.0.0.2"
            assert cmd[2] == "nova@10.0.0.3"


class TestCompleteScripts:
    """Tests for StartCommand.complete_script"""

    def test_complete_scripts(self, mock_launch_dir):
        """Test script completion"""
        with patch('nova_cli.ros2_utils.get_build_path', return_value=mock_launch_dir):
            scripts = StartCommand.complete_script("run-", None)
            assert "run-gui" in scripts
            assert "run-drive" in scripts

    def test_complete_scripts_prefix_filter(self, mock_launch_dir):
        """Test script completion filters by prefix"""
        with patch('nova_cli.ros2_utils.get_build_path', return_value=mock_launch_dir):
            scripts = StartCommand.complete_script("run-g", None)
            assert "run-gui" in scripts
            assert "run-drive" not in scripts

    def test_complete_scripts_no_launch_dir(self, tmp_path):
        """Test completion when launch dir doesn't exist"""
        with patch('nova_cli.ros2_utils.get_build_path', return_value=tmp_path):
            scripts = StartCommand.complete_script("", None)
            assert scripts == []
