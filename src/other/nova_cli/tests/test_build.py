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
        cmd = nova_cli(["build", "main"])
        assert cmd == [
            "bash", "-ic",
            "ws-build -o /home/test/Builds/main"
        ]

    def test_build_auto(self, nova_cli):
        cmd = nova_cli(["build", "auto"])
        assert cmd == [
            "bash", "-ic",
            "ws-build -o /home/test/Builds/auto"
        ]

    def test_build_with_extra_args(self, nova_cli):
        cmd = nova_cli(["build", "test", "--packages-select", "science"])
        assert cmd == [
            "bash", "-ic",
            "ws-build -o /home/test/Builds/test --packages-select science"
        ]


class TestBuildCommand:
    """Unit tests for BuildCommand error handling."""

    def test_nom_build_not_found(self, capsys):
        args = SimpleNamespace(buildname='main', extra_args=[])

        with patch.object(Path, 'home', return_value=Path('/home/test')):
            with patch('subprocess.run', side_effect=FileNotFoundError):
                result = BuildCommand.execute(args)
                assert result == 1
                assert 'nom-build not found' in capsys.readouterr().err
