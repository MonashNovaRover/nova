"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Tests for rebuild-main command.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        nova_cli
AUTHOR(S):      Nova Team
CREATION:       09/07/2026
EDITED:         09/07/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import pytest
from unittest.mock import patch, MagicMock
from types import SimpleNamespace
from pathlib import Path

from nova_cli.commands.rebuild_main import RebuildMainCommand


@pytest.fixture
def mock_home():
    """Patch Path.home() to return predictable path."""
    with patch.object(Path, 'home', return_value=Path('/home/test')):
        yield


@pytest.fixture
def mock_args():
    def _make(stash=False, extra=[]):
        return SimpleNamespace(stash=stash, extra_args=extra)
    return _make


class TestRebuildMainCommand:
    """Test nova rebuild-main command."""

    def test_successful_rebuild(self, mock_home, mock_args):
        """Test successful rebuild with no uncommitted changes."""
        with patch('subprocess.run') as mock_run, \
             patch('pathlib.Path.exists', return_value=True):
            # Mock git status: no changes
            mock_run.return_value = MagicMock(
                returncode=0,
                stdout='',
                stderr=''
            )

            # Mock all subsequent commands to succeed
            def run_side_effect(cmd, **kwargs):
                if 'status' in cmd:
                    return MagicMock(returncode=0, stdout='')
                return MagicMock(returncode=0)

            mock_run.side_effect = run_side_effect

            result = RebuildMainCommand.execute(mock_args())

            assert result == 0
            assert mock_run.call_count == 4  # status, checkout, pull, build

    def test_uncommitted_changes_without_stash_flag(self, mock_home, mock_args, capsys):
        """Test error when uncommitted changes exist and --stash not provided."""
        with patch('subprocess.run') as mock_run, \
             patch('pathlib.Path.exists', return_value=True):
            # Mock git status: has changes
            mock_run.return_value = MagicMock(
                returncode=0,
                stdout='M src/some/file.py\n'
            )

            result = RebuildMainCommand.execute(mock_args(stash=False))

            assert result == 1
            captured = capsys.readouterr()
            assert 'Uncommitted changes detected' in captured.err
            assert '--stash' in captured.err

    def test_uncommitted_changes_with_stash_flag(self, mock_home, mock_args):
        """Test auto-stash when --stash flag provided."""
        with patch('subprocess.run') as mock_run, \
             patch('pathlib.Path.exists', return_value=True):
            call_count = [0]

            def run_side_effect(cmd, **kwargs):
                call_count[0] += 1
                if 'status' in cmd:
                    return MagicMock(returncode=0, stdout='M file.py\n')
                return MagicMock(returncode=0)

            mock_run.side_effect = run_side_effect

            result = RebuildMainCommand.execute(mock_args(stash=True))

            assert result == 0
            # Verify stash was called with -u flag
            stash_calls = [call for call in mock_run.call_args_list
                          if 'stash' in str(call.args[0])]
            assert len(stash_calls) == 1
            assert '-u' in stash_calls[0].args[0]

    def test_git_status_fails(self, mock_home, mock_args, capsys):
        """Test error when git status command fails."""
        with patch('subprocess.run') as mock_run, \
             patch('pathlib.Path.exists', return_value=True):
            mock_run.return_value = MagicMock(returncode=1)

            result = RebuildMainCommand.execute(mock_args())

            assert result == 1
            captured = capsys.readouterr()
            assert 'Failed to check git status' in captured.err

    def test_git_checkout_fails(self, mock_home, mock_args, capsys):
        """Test error when git checkout fails."""
        with patch('subprocess.run') as mock_run, \
             patch('pathlib.Path.exists', return_value=True):
            def run_side_effect(cmd, **kwargs):
                if 'status' in cmd:
                    return MagicMock(returncode=0, stdout='')
                if 'checkout' in cmd:
                    return MagicMock(returncode=1)
                return MagicMock(returncode=0)

            mock_run.side_effect = run_side_effect

            result = RebuildMainCommand.execute(mock_args())

            assert result == 1
            captured = capsys.readouterr()
            assert 'Failed to checkout main branch' in captured.err

    def test_git_pull_fails(self, mock_home, mock_args, capsys):
        """Test error when git pull fails."""
        with patch('subprocess.run') as mock_run, \
             patch('pathlib.Path.exists', return_value=True):
            def run_side_effect(cmd, **kwargs):
                if 'status' in cmd:
                    return MagicMock(returncode=0, stdout='')
                if 'pull' in cmd:
                    return MagicMock(returncode=1)
                return MagicMock(returncode=0)

            mock_run.side_effect = run_side_effect

            result = RebuildMainCommand.execute(mock_args())

            assert result == 1
            captured = capsys.readouterr()
            assert 'Failed to pull latest changes' in captured.err

    def test_nom_build_not_found(self, mock_home, mock_args, capsys):
        """Test error when nom-build is not in PATH."""
        with patch('subprocess.run') as mock_run, \
             patch('pathlib.Path.exists', return_value=True):
            def run_side_effect(cmd, **kwargs):
                if 'bash' in cmd:
                    raise FileNotFoundError()
                if 'status' in cmd:
                    return MagicMock(returncode=0, stdout='')
                return MagicMock(returncode=0)

            mock_run.side_effect = run_side_effect

            result = RebuildMainCommand.execute(mock_args())

            assert result == 1
            captured = capsys.readouterr()
            assert 'nom-build not found' in captured.err

    def test_build_fails(self, mock_home, mock_args, capsys):
        """Test error when nom-build fails."""
        with patch('subprocess.run') as mock_run, \
             patch('pathlib.Path.exists', return_value=True):
            def run_side_effect(cmd, **kwargs):
                if 'bash' in cmd:
                    return MagicMock(returncode=5)
                if 'status' in cmd:
                    return MagicMock(returncode=0, stdout='')
                return MagicMock(returncode=0)

            mock_run.side_effect = run_side_effect

            result = RebuildMainCommand.execute(mock_args())

            assert result == 5  # Propagate nom-build's exit code
            captured = capsys.readouterr()
            assert 'Build failed' in captured.err

    def test_with_extra_args(self, mock_home, mock_args):
        """Test passing extra args to nom-build."""
        with patch('subprocess.run') as mock_run, \
             patch('pathlib.Path.exists', return_value=True):
            def run_side_effect(cmd, **kwargs):
                if 'status' in cmd:
                    return MagicMock(returncode=0, stdout='')
                return MagicMock(returncode=0)

            mock_run.side_effect = run_side_effect

            result = RebuildMainCommand.execute(
                mock_args(extra=['--packages-select', 'science'])
            )

            assert result == 0
            # Verify extra args passed to nom-build
            build_call = [call for call in mock_run.call_args_list
                         if 'ws-build' in str(call)]
            assert len(build_call) == 1
            assert '--packages-select' in build_call[0].args[0][2]
            assert 'science' in build_call[0].args[0][2]

    def test_nova_directory_missing(self, mock_home, mock_args, capsys):
        """Test error when ~/nova directory doesn't exist."""
        with patch.object(Path, 'exists', return_value=False):
            result = RebuildMainCommand.execute(mock_args())

            assert result == 1
            captured = capsys.readouterr()
            assert 'Nova directory not found' in captured.err

    def test_stash_fails(self, mock_home, mock_args, capsys):
        """Test error when git stash command fails."""
        with patch('subprocess.run') as mock_run, \
             patch('pathlib.Path.exists', return_value=True):
            def run_side_effect(cmd, **kwargs):
                if 'status' in cmd:
                    return MagicMock(returncode=0, stdout='M file.py\n')
                if 'stash' in cmd:
                    return MagicMock(returncode=1)
                return MagicMock(returncode=0)

            mock_run.side_effect = run_side_effect

            result = RebuildMainCommand.execute(mock_args(stash=True))

            assert result == 1
            captured = capsys.readouterr()
            assert 'Failed to stash changes' in captured.err
