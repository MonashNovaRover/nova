# Nova CLI

ROS2 wrapper CLI tool for Nova Rover that simplifies common operations and replaces ~40-50 shell aliases.

## Features

- **Automatic transformations**: `nova launch science urc` → `ros2 launch science_bringup urc.launch.py`
- **Teleop shorthand**: `nova launch teleop drive` → `ros2 launch teleop_drive teleop.launch.py`
- **Build selection**: Use `-b/--build` to select different builds
- **Bash completion**: Tab-complete packages, launch files, and executables
- **Helpful errors**: Suggests alternatives when packages/files not found

## Project Structure

```
nova_cli/
├── nova_cli/              # Main package
│   ├── __init__.py        # Package version
│   ├── main.py            # Entry point and argument parsing
│   ├── commands/          # Command implementations
│   │   ├── __init__.py    # Command registry
│   │   ├── base.py        # Abstract Command class
│   │   └── *.py           # Individual commands
│   ├── build_utils.py     # Build path validation and listing
│   ├── ros2_utils.py      # ROS2 package/executable discovery
│   └── utils.py           # General utilities (fuzzy matching)
└── tests/                 # Unit tests
    └── test_*.py          # Test files for each command
```

## Commands

| Command | Implementation | Description |
|---------|----------------|-------------|
| `build` | [build.py](nova_cli/commands/build.py) | Build workspace to `~/Builds/<name>` using ws-build |
| `env` | [env.py](nova_cli/commands/env.py) | Manage environment variables (COMP, RMW, domain) and active build |
| `launch` | [launch.py](nova_cli/commands/launch.py) | Launch ROS2 launch files with automatic `_bringup` and `.launch.py` handling |
| `rebuild-master` | [rebuild_master.py](nova_cli/commands/rebuild_master.py) | Checkout master, pull latest changes, and build to `~/Builds/master` |
| `run` | [run.py](nova_cli/commands/run.py) | Run ROS2 node executables with automatic `.py` extension handling |
| `sleuth` | [sleuth.py](nova_cli/commands/sleuth.py) | CAN bus tracer/emulator (passthrough to can_sleuth) |
| `start` | [start.py](nova_cli/commands/start.py) | Execute terminal launch scripts from `~/Builds/<build>/launch/` |

## Development

See [CONTRIBUTING.md](CONTRIBUTING.md) for:
- Adding new commands
- Understanding the utility modules
- Running tests
