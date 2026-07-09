"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Tests for build command.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        nova_cli
AUTHOR(S):      Felicity Matthews
CREATION:       09/07/2026
EDITED:         09/07/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import pytest
from unittest.mock import patch, MagicMock
from types import SimpleNamespace
from pathlib import Path

from nova_cli.commands.build import BuildCommand


@pytest.fixture
def mock_home():
    """Patch Path.home() to return predictable path."""
    with patch.object(Path, 'home', return_value=Path('/home/test')):
        yield


@pytest.fixture
def mock_args():
    def _make(buildname, extra=[]):
        return SimpleNamespace(buildname=buildname, extra_args=extra)
    return _make


class TestBuildCommand:
    """Test nova build command generates correct ws-build commands."""

    def test_basic_build(self, mock_home, mock_args):
        with patch('subprocess.run') as mock_run:
            mock_run.return_value = MagicMock(returncode=0)
            BuildCommand.execute(mock_args('master'))
            cmd = mock_run.call_args[0][0]
            assert cmd == ['ws-build', '-o', '/home/test/Builds/master']

    def test_build_auto(self, mock_home, mock_args):
        with patch('subprocess.run') as mock_run:
            mock_run.return_value = MagicMock(returncode=0)
            BuildCommand.execute(mock_args('auto'))
            cmd = mock_run.call_args[0][0]
            assert cmd == ['ws-build', '-o', '/home/test/Builds/auto']

    def test_build_with_extra_args(self, mock_home, mock_args):
        with patch('subprocess.run') as mock_run:
            mock_run.return_value = MagicMock(returncode=0)
            BuildCommand.execute(mock_args('test', ['--packages-select', 'science']))
            cmd = mock_run.call_args[0][0]
            assert cmd == ['ws-build', '-o', '/home/test/Builds/test', '--packages-select', 'science']

    def test_ws_build_not_found(self, mock_home, mock_args, capsys):
        with patch('subprocess.run', side_effect=FileNotFoundError):
            result = BuildCommand.execute(mock_args('master'))
            assert result == 1
            assert 'ws-build not found' in capsys.readouterr().err
