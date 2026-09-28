"""Tests for main.py"""
import pytest
from nova_cli.main import create_parser


class TestCreateParser:
    """Tests for create_parser function"""

    def test_parser_creation(self):
        """Parser is created with expected structure"""
        parser = create_parser()
        assert parser.prog == 'nova'

    def test_subcommands_registered(self):
        """Launch and run subcommands are registered"""
        parser = create_parser()
        # Parse a valid command to verify subcommands exist
        args, _ = parser.parse_known_args(['launch', 'science'])
        assert args.command == 'launch'

        args, _ = parser.parse_known_args(['run', 'science', 'kiln'])
        assert args.command == 'run'

    def test_build_flag_on_launch(self):
        """Build flag is available on launch command"""
        parser = create_parser()
        args, _ = parser.parse_known_args(['launch', 'science', '-b', 'main'])
        assert args.build == 'main'

    def test_build_flag_on_run(self):
        """Build flag is available on run command"""
        parser = create_parser()
        args, _ = parser.parse_known_args(['run', 'science', 'kiln', '-b', 'auto'])
        assert args.build == 'auto'

    def test_build_default(self):
        """Default build value is 'active'"""
        parser = create_parser()
        args, _ = parser.parse_known_args(['launch', 'science'])
        assert args.build == 'active'

    def test_no_global_build_flag(self):
        """Build flag before command causes an error"""
        parser = create_parser()
        # -b before command is now invalid - argparse will treat 'main' as the command
        # and fail because 'main' is not a valid subcommand
        with pytest.raises(SystemExit):
            parser.parse_known_args(['-b', 'main', 'launch', 'science'])
