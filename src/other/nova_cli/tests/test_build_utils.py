"""Tests for build_utils.py"""
import pytest
from pathlib import Path
from unittest.mock import patch

from nova_cli.build_utils import (
    validate_build_path,
    list_available_builds,
    get_ros2_executable,
    BUILDS_DIR,
)


class TestValidateBuildPath:
    """Tests for validate_build_path function"""

    def test_valid_build_path(self, tmp_path):
        """Returns path when build exists with ros2 binary"""
        build_dir = tmp_path / "test_build"
        bin_dir = build_dir / "bin"
        bin_dir.mkdir(parents=True)
        ros2_bin = bin_dir / "ros2"
        ros2_bin.touch()

        with patch.object(Path, 'home', return_value=tmp_path.parent):
            with patch('nova_cli.build_utils.BUILDS_DIR', tmp_path):
                result = validate_build_path("test_build")
                assert result == build_dir

    def test_missing_build_directory(self, tmp_path):
        """Returns None when build directory doesn't exist"""
        with patch('nova_cli.build_utils.BUILDS_DIR', tmp_path):
            result = validate_build_path("nonexistent")
            assert result is None

    def test_missing_ros2_binary(self, tmp_path):
        """Returns None when ros2 binary doesn't exist"""
        build_dir = tmp_path / "test_build"
        bin_dir = build_dir / "bin"
        bin_dir.mkdir(parents=True)
        # Don't create ros2 binary

        with patch('nova_cli.build_utils.BUILDS_DIR', tmp_path):
            result = validate_build_path("test_build")
            assert result is None


class TestListAvailableBuilds:
    """Tests for list_available_builds function"""

    def test_lists_valid_builds(self, tmp_path):
        """Lists only directories with ros2 binary"""
        # Create valid build
        valid_build = tmp_path / "master"
        (valid_build / "bin").mkdir(parents=True)
        (valid_build / "bin" / "ros2").touch()

        # Create invalid build (no ros2)
        invalid_build = tmp_path / "broken"
        invalid_build.mkdir()

        with patch('nova_cli.build_utils.BUILDS_DIR', tmp_path):
            result = list_available_builds()
            assert result == ["master"]

    def test_empty_builds_dir(self, tmp_path):
        """Returns empty list when no builds exist"""
        with patch('nova_cli.build_utils.BUILDS_DIR', tmp_path):
            result = list_available_builds()
            assert result == []

    def test_nonexistent_builds_dir(self, tmp_path):
        """Returns empty list when builds dir doesn't exist"""
        nonexistent = tmp_path / "nonexistent"
        with patch('nova_cli.build_utils.BUILDS_DIR', nonexistent):
            result = list_available_builds()
            assert result == []

    def test_sorted_output(self, tmp_path):
        """Builds are returned in sorted order"""
        for name in ["drive", "auto", "master", "arm"]:
            build = tmp_path / name
            (build / "bin").mkdir(parents=True)
            (build / "bin" / "ros2").touch()

        with patch('nova_cli.build_utils.BUILDS_DIR', tmp_path):
            result = list_available_builds()
            assert result == ["arm", "auto", "drive", "master"]

    def test_includes_symlinks(self, tmp_path):
        """Symlinks with ros2 binary are included"""
        # Create actual build
        real_build = tmp_path / "real_build"
        (real_build / "bin").mkdir(parents=True)
        (real_build / "bin" / "ros2").touch()

        # Create symlink
        symlink = tmp_path / "active"
        symlink.symlink_to(real_build)

        with patch('nova_cli.build_utils.BUILDS_DIR', tmp_path):
            result = list_available_builds()
            assert "active" in result
            assert "real_build" in result


class TestGetRos2Executable:
    """Tests for get_ros2_executable function"""

    def test_returns_correct_path(self, tmp_path):
        """Returns path to ros2 binary"""
        result = get_ros2_executable(tmp_path)
        assert result == tmp_path / "bin" / "ros2"
