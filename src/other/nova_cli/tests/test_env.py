"""Tests for env command"""
import os
import pytest
from pathlib import Path
from unittest.mock import patch, MagicMock

from nova_cli.commands.env import (
    EnvCommand,
    EnvVar,
    ENV_VARS,
    CONFIG_DIR,
    get_active_build,
    set_active_build,
)


class TestEnvStatus:
    """Tests for nova env / nova env status."""

    def test_status_shows_all_vars(self, capsys, tmp_path):
        with patch.object(EnvCommand, '_get_value', return_value="test_value"):
            EnvCommand._status()
            output = capsys.readouterr().out
            for var_name in ENV_VARS.keys():
                assert var_name in output

    def test_status_shows_build_with_arrow(self, capsys):
        with patch.object(EnvCommand, '_get_value', side_effect=lambda v: "master" if v.name == "build" else "(not set)"):
            EnvCommand._status()
            output = capsys.readouterr().out
            assert "build:   active -> master" in output


class TestEnvSet:
    """Tests for nova env set."""

    def test_set_standard_var(self, tmp_path, capsys):
        config_dir = tmp_path / ".config" / "nova"
        with patch('nova_cli.commands.env.CONFIG_DIR', config_dir):
            result = EnvCommand._set("comp", "URC")
            assert result == 0

            # Check stdout (for eval)
            stdout = capsys.readouterr().out
            assert "export COMP=URC" in stdout

            # Check config file was written
            config_file = config_dir / "comp"
            assert config_file.exists()
            assert config_file.read_text() == "export COMP=URC\n"

    def test_set_with_shortcut(self, tmp_path, capsys):
        config_dir = tmp_path / ".config" / "nova"
        with patch('nova_cli.commands.env.CONFIG_DIR', config_dir):
            result = EnvCommand._set("rmw", "cyclone")
            assert result == 0

            stdout = capsys.readouterr().out
            assert "export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp" in stdout

    def test_set_build_calls_setter(self, tmp_path):
        builds_dir = tmp_path / "Builds"
        master_build = builds_dir / "master" / "bin"
        master_build.mkdir(parents=True)
        (master_build / "ros2").touch()

        with patch('nova_cli.commands.env.BUILDS_DIR', builds_dir):
            result = set_active_build("master")
            assert result == 0
            assert (builds_dir / "active").is_symlink()
            assert (builds_dir / "active").resolve() == builds_dir / "master"

    def test_set_build_nonexistent(self, tmp_path, capsys):
        builds_dir = tmp_path / "Builds"
        builds_dir.mkdir(parents=True)

        with patch('nova_cli.commands.env.BUILDS_DIR', builds_dir):
            result = set_active_build("nonexistent")
            assert result == 1
            stderr = capsys.readouterr().err
            assert "not found" in stderr


class TestEnvGet:
    """Tests for nova env get."""

    def test_get_from_config_file(self, tmp_path):
        config_dir = tmp_path / ".config" / "nova"
        config_dir.mkdir(parents=True)
        (config_dir / "comp").write_text("export COMP=URC\n")

        var = ENV_VARS["comp"]
        with patch('nova_cli.commands.env.CONFIG_DIR', config_dir):
            value = EnvCommand._get_value(var)
            assert value == "URC"

    def test_get_from_env_var(self, tmp_path):
        config_dir = tmp_path / ".config" / "nova"
        var = ENV_VARS["comp"]

        with patch('nova_cli.commands.env.CONFIG_DIR', config_dir):
            with patch.dict(os.environ, {"COMP": "CIRC"}):
                value = EnvCommand._get_value(var)
                assert value == "CIRC"

    def test_get_not_set(self, tmp_path):
        config_dir = tmp_path / ".config" / "nova"
        var = ENV_VARS["comp"]

        with patch('nova_cli.commands.env.CONFIG_DIR', config_dir):
            with patch.dict(os.environ, {}, clear=True):
                # Also need to ensure COMP isn't in the environment
                os.environ.pop("COMP", None)
                value = EnvCommand._get_value(var)
                assert value == "(not set)"

    def test_get_build_uses_getter(self, tmp_path):
        builds_dir = tmp_path / "Builds"
        master_build = builds_dir / "master" / "bin"
        master_build.mkdir(parents=True)
        (master_build / "ros2").touch()

        active = builds_dir / "active"
        active.symlink_to(builds_dir / "master")

        with patch('nova_cli.commands.env.BUILDS_DIR', builds_dir):
            value = get_active_build()
            assert value == "master"

    def test_get_build_not_set(self, tmp_path):
        builds_dir = tmp_path / "Builds"
        builds_dir.mkdir(parents=True)

        with patch('nova_cli.commands.env.BUILDS_DIR', builds_dir):
            value = get_active_build()
            assert value == "(not set)"


class TestEnvList:
    """Tests for nova env list."""

    def test_list_shows_all_vars(self, capsys):
        EnvCommand._list()
        output = capsys.readouterr().out

        for var_name, var in ENV_VARS.items():
            assert var_name in output
            assert var.description in output

    def test_list_shows_shortcuts(self, capsys):
        EnvCommand._list()
        output = capsys.readouterr().out

        # rmw has shortcuts, they should be displayed
        assert "shortcuts:" in output
        assert "cyclone" in output


class TestEnvVarRegistry:
    """Tests for the EnvVar registry."""

    def test_all_standard_vars_have_config_file(self):
        for var_name, var in ENV_VARS.items():
            if var.env_var is not None:
                assert var.config_file is not None, f"{var_name} has env_var but no config_file"

    def test_build_var_has_custom_getter_setter(self):
        build_var = ENV_VARS["build"]
        assert build_var.getter is not None
        assert build_var.setter is not None
        assert build_var.env_var is None

    def test_rmw_shortcuts_expand_correctly(self):
        rmw_var = ENV_VARS["rmw"]
        assert rmw_var.value_shortcuts["cyclone"] == "rmw_cyclonedds_cpp"
        assert rmw_var.value_shortcuts["fastrtps"] == "rmw_fastrtps_cpp"
        assert rmw_var.value_shortcuts["fast"] == "rmw_fastrtps_cpp"


class TestEnvCommandParser:
    """Tests for command parser setup."""

    def test_parser_has_subcommands(self):
        import argparse
        parser = argparse.ArgumentParser()
        subparsers = parser.add_subparsers(dest='command')
        EnvCommand.add_parser(subparsers)

        # Parse 'env status'
        args = parser.parse_args(['env', 'status'])
        assert args.env_action == 'status'

        # Parse 'env set comp urc'
        args = parser.parse_args(['env', 'set', 'comp', 'urc'])
        assert args.env_action == 'set'
        assert args.var == 'comp'
        assert args.value == 'urc'

        # Parse 'env get comp'
        args = parser.parse_args(['env', 'get', 'comp'])
        assert args.env_action == 'get'
        assert args.var == 'comp'

        # Parse 'env list'
        args = parser.parse_args(['env', 'list'])
        assert args.env_action == 'list'

    def test_parser_env_no_action_defaults_to_status(self):
        import argparse
        parser = argparse.ArgumentParser()
        subparsers = parser.add_subparsers(dest='command')
        EnvCommand.add_parser(subparsers)

        args = parser.parse_args(['env'])
        assert args.env_action is None  # Will default to status in execute()


class TestEnvExecute:
    """Tests for execute method routing."""

    def test_execute_no_action_calls_status(self):
        from types import SimpleNamespace
        args = SimpleNamespace(env_action=None)
        with patch.object(EnvCommand, '_status', return_value=0) as mock:
            EnvCommand.execute(args)
            mock.assert_called_once()

    def test_execute_status_calls_status(self):
        from types import SimpleNamespace
        args = SimpleNamespace(env_action='status')
        with patch.object(EnvCommand, '_status', return_value=0) as mock:
            EnvCommand.execute(args)
            mock.assert_called_once()

    def test_execute_set_calls_set(self):
        from types import SimpleNamespace
        args = SimpleNamespace(env_action='set', var='comp', value='URC')
        with patch.object(EnvCommand, '_set', return_value=0) as mock:
            EnvCommand.execute(args)
            mock.assert_called_once_with('comp', 'URC')

    def test_execute_get_calls_get(self):
        from types import SimpleNamespace
        args = SimpleNamespace(env_action='get', var='comp')
        with patch.object(EnvCommand, '_get', return_value=0) as mock:
            EnvCommand.execute(args)
            mock.assert_called_once_with('comp')

    def test_execute_list_calls_list(self):
        from types import SimpleNamespace
        args = SimpleNamespace(env_action='list')
        with patch.object(EnvCommand, '_list', return_value=0) as mock:
            EnvCommand.execute(args)
            mock.assert_called_once()
