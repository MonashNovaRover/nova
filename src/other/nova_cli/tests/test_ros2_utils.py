"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Tests for ros2_utils module.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        nova_cli
AUTHOR(S):      Felicity Matthews
CREATION:       20/07/2026
EDITED:         20/07/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
from pathlib import Path
from unittest.mock import patch, MagicMock

from nova_cli import ros2_utils


class TestGetBuildPath:
    """Tests for get_build_path."""

    def test_returns_active_build(self):
        with patch.object(Path, 'home', return_value=Path('/home/test')):
            result = ros2_utils.get_build_path()
            assert result == Path('/home/test/Builds/active')


class TestListPackages:
    """Tests for list_packages."""

    def test_lists_packages(self, mock_build_path):
        # Create mock share directory structure
        share_dir = mock_build_path / "share"
        share_dir.mkdir(parents=True)
        (share_dir / "science_bringup").mkdir()
        (share_dir / "drive_bringup").mkdir()
        (share_dir / "auto_bringup").mkdir()

        result = ros2_utils.list_packages(mock_build_path)
        assert result == ['auto_bringup', 'drive_bringup', 'science_bringup']

    def test_returns_empty_when_share_missing(self, mock_build_path):
        # No share directory exists
        assert ros2_utils.list_packages(mock_build_path) == []

    def test_ignores_files(self, mock_build_path):
        # Create share directory with both dirs and files
        share_dir = mock_build_path / "share"
        share_dir.mkdir(parents=True)
        (share_dir / "science").mkdir()
        (share_dir / "readme.txt").touch()  # Should be ignored

        result = ros2_utils.list_packages(mock_build_path)
        assert result == ['science']
        assert 'readme.txt' not in result


class TestPackageExists:
    """Tests for package_exists."""

    def test_package_found(self, mock_build_path):
        # Create mock share directory structure
        share_dir = mock_build_path / "share"
        share_dir.mkdir(parents=True)
        (share_dir / "science_bringup").mkdir()
        (share_dir / "drive_bringup").mkdir()

        assert ros2_utils.package_exists(mock_build_path, 'science_bringup') is True

    def test_package_not_found(self, mock_build_path):
        # Create mock share directory structure without science_bringup
        share_dir = mock_build_path / "share"
        share_dir.mkdir(parents=True)
        (share_dir / "drive_bringup").mkdir()

        assert ros2_utils.package_exists(mock_build_path, 'science_bringup') is False

    def test_share_dir_missing(self, mock_build_path):
        # No share directory exists
        assert ros2_utils.package_exists(mock_build_path, 'science_bringup') is False


class TestListExecutables:
    """Tests for list_executables."""

    def test_lists_executables(self, mock_build_path):
        # Create mock lib directory structure
        lib_dir = mock_build_path / "lib" / "science"
        lib_dir.mkdir(parents=True)

        # Create executable files
        (lib_dir / "kiln").touch(mode=0o755)
        (lib_dir / "camera").touch(mode=0o755)
        (lib_dir / "sensor.py").touch(mode=0o755)

        # Create non-executable file (should be ignored)
        (lib_dir / "readme.txt").touch(mode=0o644)

        result = ros2_utils.list_executables(mock_build_path, 'science')
        assert result == ['camera', 'kiln', 'sensor.py']

    def test_handles_empty_directory(self, mock_build_path):
        # Create empty lib directory
        lib_dir = mock_build_path / "lib" / "science"
        lib_dir.mkdir(parents=True)

        assert ros2_utils.list_executables(mock_build_path, 'science') == []

    def test_handles_missing_directory(self, mock_build_path):
        # No lib directory exists
        assert ros2_utils.list_executables(mock_build_path, 'science') == []


class TestListLaunchFiles:
    """Tests for list_launch_files."""

    def test_lists_launch_files(self, mock_build_path):
        # Create mock launch directory structure
        launch_dir = mock_build_path / "share" / "science_bringup" / "launch"
        launch_dir.mkdir(parents=True)
        (launch_dir / "urc.launch.py").touch()
        (launch_dir / "sim.launch.py").touch()

        result = ros2_utils.list_launch_files(mock_build_path, 'science_bringup')
        assert result == ['sim', 'urc']

    def test_strips_launch_suffix(self, mock_build_path):
        # Files named *.launch.launch.py should become just the base name
        launch_dir = mock_build_path / "share" / "drive_bringup" / "launch"
        launch_dir.mkdir(parents=True)
        (launch_dir / "drive.launch.py").touch()

        result = ros2_utils.list_launch_files(mock_build_path, 'drive_bringup')
        assert result == ['drive']

    def test_handles_missing_directory(self, mock_build_path):
        # No launch directory exists
        assert ros2_utils.list_launch_files(mock_build_path, 'nonexistent') == []


class TestListScripts:
    """Tests for list_scripts."""

    def test_lists_executable_scripts(self, tmp_path):
        # Create executable scripts
        (tmp_path / "run-gui").touch(mode=0o755)
        (tmp_path / "run-drive").touch(mode=0o755)

        # Create non-executable file (should be ignored)
        (tmp_path / "readme.txt").touch(mode=0o644)

        result = ros2_utils.list_scripts(tmp_path)
        assert result == ['run-drive', 'run-gui']

    def test_handles_missing_directory(self, tmp_path):
        nonexistent = tmp_path / "nonexistent"
        assert ros2_utils.list_scripts(nonexistent) == []


class TestRunRos2Command:
    """Tests for run_ros2_command."""

    def test_runs_ros2_command(self, mock_build_path, capsys):
        mock_result = MagicMock()
        mock_result.returncode = 0

        with patch('subprocess.run', return_value=mock_result) as mock_run:
            result = ros2_utils.run_ros2_command(mock_build_path, ['launch', 'pkg', 'file'])
            assert result == 0
            mock_run.assert_called_once()
            cmd = mock_run.call_args[0][0]
            assert cmd[0] == str(mock_build_path / "bin" / "ros2")
            assert cmd[1:] == ['launch', 'pkg', 'file']

    def test_prints_command(self, mock_build_path, capsys):
        mock_result = MagicMock()
        mock_result.returncode = 0

        with patch('subprocess.run', return_value=mock_result):
            ros2_utils.run_ros2_command(mock_build_path, ['pkg', 'list'])
            stderr = capsys.readouterr().err
            assert 'Running:' in stderr
            assert 'ros2' in stderr

    def test_returns_nonzero_on_failure(self, mock_build_path):
        mock_result = MagicMock()
        mock_result.returncode = 1

        with patch('subprocess.run', return_value=mock_result):
            result = ros2_utils.run_ros2_command(mock_build_path, ['pkg', 'list'])
            assert result == 1
