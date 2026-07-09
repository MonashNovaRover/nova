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


class TestBuildCLI:
    """Full CLI tests for nova build command."""

    def test_basic_build(self, nova_cli):
        cmd = nova_cli(["build", "master"])
        assert cmd == ["ws-build", "-o", "/home/test/Builds/master"]

    def test_build_auto(self, nova_cli):
        cmd = nova_cli(["build", "auto"])
        assert cmd == ["ws-build", "-o", "/home/test/Builds/auto"]

    def test_build_with_extra_args(self, nova_cli):
        cmd = nova_cli(["build", "test", "--packages-select", "science"])
        assert cmd == ["ws-build", "-o", "/home/test/Builds/test", "--packages-select", "science"]


class TestBuildCommand:
    """Unit tests for BuildCommand error handling."""

    def test_ws_build_not_found(self, capsys):
        args = SimpleNamespace(buildname='master', extra_args=[])

        with patch.object(Path, 'home', return_value=Path('/home/test')):
            with patch('subprocess.run', side_effect=FileNotFoundError):
                result = BuildCommand.execute(args)
                assert result == 1
                assert 'ws-build not found' in capsys.readouterr().err
