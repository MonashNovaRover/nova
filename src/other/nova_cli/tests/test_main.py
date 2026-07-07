"""Tests for main.py"""
import pytest
from nova_cli.main import extract_global_flags, create_parser


class TestExtractGlobalFlags:
    """Tests for extract_global_flags function"""

    def test_no_flags(self):
        """No global flags returns default build"""
        global_flags, filtered = extract_global_flags(['launch', 'science'])
        assert global_flags == {'build': 'active'}
        assert filtered == ['launch', 'science']

    def test_build_flag_at_start(self):
        """-b flag at start of argv"""
        global_flags, filtered = extract_global_flags(['-b', 'master', 'launch', 'science'])
        assert global_flags == {'build': 'master'}
        assert filtered == ['launch', 'science']

    def test_build_flag_at_end(self):
        """-b flag at end of argv"""
        global_flags, filtered = extract_global_flags(['launch', 'science', '-b', 'auto'])
        assert global_flags == {'build': 'auto'}
        assert filtered == ['launch', 'science']

    def test_build_flag_in_middle(self):
        """-b flag in middle of argv"""
        global_flags, filtered = extract_global_flags(['launch', '-b', 'arm', 'science', 'urc'])
        assert global_flags == {'build': 'arm'}
        assert filtered == ['launch', 'science', 'urc']

    def test_long_form_build_flag(self):
        """--build long form flag works"""
        global_flags, filtered = extract_global_flags(['--build', 'drive', 'launch', 'science'])
        assert global_flags == {'build': 'drive'}
        assert filtered == ['launch', 'science']

    def test_empty_argv(self):
        """Empty argv returns defaults"""
        global_flags, filtered = extract_global_flags([])
        assert global_flags == {'build': 'active'}
        assert filtered == []

    def test_preserves_extra_args(self):
        """Preserves ROS2 style extra args"""
        global_flags, filtered = extract_global_flags(
            ['launch', 'auto', 'sim', '-b', 'master', 'sim:=True']
        )
        assert global_flags == {'build': 'master'}
        assert filtered == ['launch', 'auto', 'sim', 'sim:=True']

    def test_multiple_non_flag_args(self):
        """Multiple args without flags preserved"""
        global_flags, filtered = extract_global_flags(
            ['run', 'science', 'kiln', '--some-arg', 'value']
        )
        assert global_flags == {'build': 'active'}
        assert filtered == ['run', 'science', 'kiln', '--some-arg', 'value']


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

    def test_build_default(self):
        """Default build value is 'active'"""
        parser = create_parser()
        args, _ = parser.parse_known_args(['launch', 'science'])
        assert args.build == 'active'
