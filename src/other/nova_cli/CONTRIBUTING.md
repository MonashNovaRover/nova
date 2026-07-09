# Contributing to Nova CLI

This guide covers how to add new commands, understand the codebase structure, and run tests.

## Project Layout

```
nova_cli/
├── nova_cli/
│   ├── __init__.py          # Package version
│   ├── main.py               # Entry point, argument parsing, command dispatch
│   ├── commands/
│   │   ├── __init__.py       # COMMANDS registry - add new commands here
│   │   ├── base.py           # Abstract Command base class
│   │   └── *.py              # Individual command implementations
│   ├── build_utils.py        # Build path validation, listing available builds
│   ├── ros2_utils.py         # ROS2 package/executable/launch file discovery
│   └── utils.py              # General utilities (fuzzy matching)
└── tests/
    ├── conftest.py           # Shared pytest fixtures
    └── test_*.py             # Test files
```

## Utility Modules

### `build_utils.py`

Build-related utilities:
- `BUILDS_DIR` - Path to `~/Builds/`
- `validate_build_path(name)` - Check if build exists and has `bin/ros2`
- `list_available_builds()` - List all valid builds
- `add_build_argument(parser)` - Add `-b/--build` to a command
- `validate_build_arg(args)` - Validate and set `args.build_path`

### `ros2_utils.py`

ROS2 discovery utilities:
- `list_packages(build_path)` - List all ROS2 packages
- `package_exists(build_path, name)` - Check if package exists
- `list_executables(build_path, pkg)` - List executables for a package
- `list_launch_files(build_path, pkg)` - List launch files for a package
- `list_scripts(launch_dir)` - List executable scripts in a directory
- `run_ros2_command(build_path, args)` - Run a ros2 command

### `utils.py`

General utilities:
- `fuzzy_match(target, options)` - Find close matches for suggestions

## Adding a New Command

### 1. Create the Command File

Create `nova_cli/commands/newcmd.py`:

```python
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Brief description of what this command does.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
EXAMPLES:
  nova newcmd arg1              # Example usage
  nova newcmd arg1 --flag       # With optional flag
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        nova_cli
AUTHOR(S):      Your Name
CREATION:       DD/MM/YYYY
EDITED:         DD/MM/YYYY
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import sys

from nova_cli.commands.base import Command
from nova_cli.build_utils import add_build_argument, validate_build_arg


class NewcmdCommand(Command):
    """Implements 'nova newcmd' command"""

    @staticmethod
    def add_parser(subparsers):
        """Add newcmd subcommand parser"""
        parser = subparsers.add_parser(
            'newcmd',
            help='Short help shown in nova --help',
            description='Longer description for nova newcmd --help'
        )

        # Add -b/--build if command uses builds
        add_build_argument(parser)

        # Add positional arguments
        arg = parser.add_argument(
            'myarg',
            help='Description of this argument'
        )
        # Optional: add tab completion
        arg.completer = NewcmdCommand.complete_myarg

        return parser

    @staticmethod
    def execute(args):
        """Execute newcmd command"""
        # Validate build if using builds
        if (err := validate_build_arg(args)) is not None:
            return err

        # Access args.extra_args for passthrough arguments
        # Implement command logic...

        return 0  # Return exit code

    @staticmethod
    def complete_myarg(prefix, parsed_args, **kwargs):
        """Tab completion for myarg."""
        options = ['option1', 'option2', 'option3']
        return [o for o in options if o.startswith(prefix)]
```

The last function `complete_myarg` is optional if you want auto-complete.

### 2. Register the Command

Edit `nova_cli/commands/__init__.py`:

```python
from nova_cli.commands.newcmd import NewcmdCommand

COMMANDS = {
    # ... existing commands ...
    'newcmd': NewcmdCommand,  # Add your new command here (in alphabetical order)
    # ... existing commands ...
}
```

### 3. Create Tests

Create `tests/test_newcmd.py`:

```python
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Tests for newcmd command.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        nova_cli
AUTHOR(S):      Your Name
CREATION:       DD/MM/YYYY
EDITED:         DD/MM/YYYY
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import pytest
from unittest.mock import patch, MagicMock
from types import SimpleNamespace
from pathlib import Path

from nova_cli.commands.newcmd import NewcmdCommand


@pytest.fixture
def mock_home():
    """Patch Path.home() to return predictable path."""
    with patch.object(Path, 'home', return_value=Path('/home/test')):
        yield


@pytest.fixture
def mock_args():
    def _make(myarg, extra=[]):
        return SimpleNamespace(myarg=myarg, extra_args=extra)
    return _make


class TestNewcmdCommand:
    """Test nova newcmd command."""

    def test_basic_usage(self, mock_home, mock_args):
        with patch('subprocess.run') as mock_run:
            mock_run.return_value = MagicMock(returncode=0)
            NewcmdCommand.execute(mock_args('value'))
            cmd = mock_run.call_args[0][0]
            assert cmd == ['expected', 'command', 'value']

    def test_error_handling(self, mock_home, mock_args, capsys):
        with patch('subprocess.run', side_effect=FileNotFoundError):
            result = NewcmdCommand.execute(mock_args('value'))
            assert result == 1
            assert 'error message' in capsys.readouterr().err
```

## Command Patterns

### Passthrough Command

For commands that wrap external tools:

```python
@staticmethod
def execute(args):
    cmd = ['external-tool'] + args.extra_args

    print(f"Running: {' '.join(cmd)}", file=sys.stderr)

    try:
        result = subprocess.run(cmd)
        return result.returncode
    except FileNotFoundError:
        print("Error: external-tool not found in PATH", file=sys.stderr)
        return 1
```

Use `add_help=False` in the parser to let the external tool handle `--help`.

### ROS2 Command

For commands that wrap ROS2:

```python
@staticmethod
def execute(args):
    if (err := validate_build_arg(args)) is not None:
        return err

    # Validate package/node exists
    if not ros2_utils.package_exists(args.build_path, package):
        print(f"Error: Package '{package}' not found.", file=sys.stderr)
        # Show suggestions
        from nova_cli.utils import fuzzy_match
        suggestions = fuzzy_match(package, available)
        if suggestions:
            print(f"Did you mean: {', '.join(suggestions)}?", file=sys.stderr)
        return 1

    # Build and run command
    ros2_args = ['launch', package, file] + args.extra_args
    return ros2_utils.run_ros2_command(args.build_path, ros2_args)
```

## Running the CLI

From the `nova_cli` directory:

```bash
# Using the mpython3 alias (if available)
mpython3 -m nova_cli <command> [args...]

# Or using the full path
~/Builds/active/bin/python3 -m nova_cli <command> [args...]
```

## Running Tests

```bash
cd src/other/nova_cli

# Run all tests
~/Builds/active/bin/python3 -m pytest

# Run with verbose output
~/Builds/active/bin/python3 -m pytest -v

# Run specific test file
~/Builds/active/bin/python3 -m pytest tests/test_<command>.py

# Run specific test
~/Builds/active/bin/python3 -m pytest tests/test_<command>.py::TestClass::test_name
```

## Bash Completion

Tab completion is handled by `argcomplete`. Completion works automatically for any command registered in `COMMANDS`.

### Adding Completion for Arguments

To add completion for a new argument:

1. Define a completer method on your command class
2. Assign it to the argument: `arg.completer = MyCommand.complete_arg`

The completer receives `(prefix, parsed_args, **kwargs)` and returns a list of matching strings.

### How Completion is Installed

The completion script is generated during the Nix build in [`default.nix:16-22`](default.nix#L16-L22):

```nix
postInstall = ''
  mkdir -p $out/share/bash-completion/completions
  ${argcomplete}/bin/register-python-argcomplete nova > $out/share/bash-completion/completions/nova
'';
```

This creates `~/Builds/active/share/bash-completion/completions/nova`.

The script is sourced automatically in the shell via [`nixfiles/modules/home/macros/default.nix:166-168`](../../nixfiles/modules/home/macros/default.nix#L166-L168):

```nix
# Source nova CLI completion
if [[ -f ~/Builds/active/share/bash-completion/completions/nova ]]; then
  . ~/Builds/active/share/bash-completion/completions/nova
```

Shell aliases are also defined there at [lines 369-371](../../nixfiles/modules/home/macros/default.nix#L369-L371):

```nix
nova = "~/Builds/active/bin/nova";
launch = "nova launch";
run = "nova run";
```

### Manual Completion (Development)

If completion isn't working during development, enable it manually:

```bash
eval "$(register-python-argcomplete nova)"
```

## Code Style

- Follow the existing file header format
- Use `sys.stderr` for status messages (stdout is for output)
- Return exit codes: 0 for success, 1 for errors
- Use fuzzy matching to suggest alternatives on errors
