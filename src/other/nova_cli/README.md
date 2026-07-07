# Nova CLI

ROS2 wrapper CLI tool for Nova Rover that simplifies common operations and replaces ~40-50 shell aliases.

## Features

- **Automatic transformations**: `nova launch science urc` → `ros2 launch science_bringup urc.launch.py`
- **Teleop shorthand**: `nova launch teleop drive` → `ros2 launch teleop_drive teleop.launch.py`
- **Build selection**: Use `-b/--build` to select different builds
- **Bash completion**: Tab-complete packages, launch files, and executables
- **Helpful errors**: Suggests alternatives when packages/files not found

## Installation

The package is automatically installed with the Nova workspace via Nix.

## Usage

### Launch Command

```bash
# Basic usage
nova launch science urc

# Omit launch file name to use package name
nova launch science           # Uses science.launch.py
nova launch drive             # Uses drive.launch.py

# With different build
nova -b auto launch auto sim

# With arguments
nova launch drive drive auto:=True
```

Automatically:
- Defaults launch file to package name if omitted (e.g., `nova launch science` → `science.launch.py`)
- Tries appending `_bringup` to package names first (falls back to exact name)
- Appends `.launch.py` to launch file names
- Passes through additional arguments

### Teleop Shorthand

```bash
# Instead of: nova launch teleop_drive teleop
# Use:        nova launch teleop drive

# Works for any subsystem
nova launch teleop drive
nova launch teleop arm
nova launch teleop science
nova launch teleop ec
```

Expands to: `ros2 launch teleop_<subsystem> teleop.launch.py`

### Run Command

```bash
# Basic usage
nova run science kiln

# With arguments
nova run science kiln --ros-args -p param:=value
```

Automatically:
- Tries exact node name first, then appends `.py` if needed
- Finds executables in the package

### Build Selection

Use `-b/--build` to select a different build. The flag can be placed anywhere:

```bash
# All of these work:
nova -b master launch science urc
nova launch science urc -b master
nova launch -b master science urc
nova -b auto launch auto sim
```

Available builds are in `~/Builds/` (active, master, auto, arm, drive, etc.)

Default build is `active` which can be configured via `set_active`.

**Note:** The `-b` flag is parsed before ROS2 parameters, so you can mix them:
```bash
nova launch drive auto:=True -b master
launch science urc -b auto comp:=URC
```

## Examples

### Replacing Old Aliases

```bash
# Old:  launch-science-urc = "~/Builds/active/bin/ros2 launch science_bringup urc.launch.py"
# New:  launch-science-urc = "nova launch science urc"

# Old:  launch-drive = "~/Builds/active/bin/ros2 launch drive_bringup drive.launch.py"
# New:  launch-drive = "nova launch drive"           # Simplified!

# Old:  launch-teleop-drive = "~/Builds/active/bin/ros2 launch teleop_drive_joy teleop.launch.py"
# New:  launch-teleop-drive = "nova launch teleop drive"

# Old:  launch-auto-drive = "~/Builds/active/bin/ros2 launch drive_bringup drive.launch.py auto:=True"
# New:  launch-auto-drive = "nova launch drive drive auto:=True"
# Or:   launch-auto-drive = "nova launch drive auto:=True"  # Even simpler!

# Old:  run-spec = "~/Builds/master/bin/ros2 run science urc_uv_vis_spec.py"
# New:  run-spec = "nova -b master run science urc_uv_vis_spec"
```

## Bash Completion

Bash completion is automatically installed via the Nix package.

To manually enable completion (if not working):
```bash
eval "$(register-python-argcomplete nova)"
```

Completion works for:
- Package names (with and without `_bringup` suffix)
- Launch file names
- Node/executable names
- Build names

## Shell Aliases

Add these aliases to your shell for convenience (already in nixfiles):

```bash
alias launch="nova launch"
alias run="nova run"
```

## Extending

To add new commands:

1. Create a new file in `nova_cli/commands/`
2. Implement a command class inheriting from `Command`
3. Add to `COMMANDS` registry in `nova_cli/commands/__init__.py`

Example:

```python
# nova_cli/commands/service.py
from nova_cli.commands.base import Command

class ServiceCommand(Command):
    @staticmethod
    def add_parser(subparsers):
        parser = subparsers.add_parser('service', help='Service operations')
        # Add arguments...
        return parser

    @staticmethod
    def execute(args):
        # Implement command...
        return 0
```

Then add to registry:

```python
# nova_cli/commands/__init__.py
from nova_cli.commands.service import ServiceCommand

COMMANDS = {
    'launch': LaunchCommand,
    'run': RunCommand,
    'service': ServiceCommand,  # New command
}
```

## Development

### Running the CLI Directly

You can run the CLI directly from the source directory without building or installing:

```bash
# From the nova_cli directory
cd src/other/nova_cli

# Run as a module (use mpython3 alias or full path)
mpython3 -m nova_cli launch science urc
# Or: ~/Builds/active/bin/python3 -m nova_cli launch science urc
```

### Running Tests

Run the test suite using pytest:

```bash
# From the nova_cli directory
cd src/other/nova_cli

# Run all tests
mpython3 -m pytest

# Run with verbose output
mpython3 -m pytest -v

# Run a specific test file
mpython3 -m pytest tests/test_launch.py

# Run a specific test
mpython3 -m pytest tests/test_launch.py::test_launch_basic
```

## Implementation Details

### Package Resolution

The `launch` command uses smart package resolution:

1. First tries appending `_bringup` (e.g., `science` → `science_bringup`)
2. If not found, tries exact name (e.g., `cameras`)
3. Shows suggestions if package not found

### Node Resolution

The `run` command uses smart node resolution:

1. First tries exact executable name
2. If not found, tries appending `.py` extension
3. Shows suggestions if node not found

### Build Validation

Build paths are validated before execution:
- Checks if `~/Builds/<build>/` exists
- Checks if `~/Builds/<build>/bin/ros2` exists
- Shows available builds if invalid build specified

## Future Enhancements

Potential commands for Phase 2:
- `nova service call <service> <args>` - Simplify service calls
- `nova control <command>` - Wrap ros2 control commands
- `nova topic <command>` - Wrap topic commands
- `nova bag <command>` - Wrap rosbag commands
- `nova status` - Show current build, RMW_IMPLEMENTATION, COMP
- `nova config` - Manage build/dds/comp settings

## License

Apache 2.0
