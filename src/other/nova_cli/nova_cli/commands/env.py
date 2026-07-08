"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Manages Nova environment variables and build configuration. Supports
setting/getting env vars like COMP, RMW_IMPLEMENTATION, ROS_DOMAIN_ID,
and switching the active build symlink.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
EXAMPLES:
  nova env status               # Show all env var values
  nova env set comp URC         # Set competition mode
  nova env set rmw cyclone      # Set RMW (with shortcut)
  nova env set build master     # Switch active build
  nova env get domain           # Get ROS domain ID
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        nova_cli
AUTHOR(S):      Felicity Matthews
CREATION:       06/07/2026
EDITED:         09/07/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Callable, Dict, List, Optional

from nova_cli.commands.base import Command
from nova_cli.utils import BUILDS_DIR


CONFIG_DIR = Path.home() / ".config" / "nova"


def get_active_build() -> str:
    """Get the name of the currently active build."""
    active = BUILDS_DIR / "active"
    if active.is_symlink():
        return active.resolve().name
    return "(not set)"


def set_active_build(build_name: str) -> int:
    """Set the active build symlink."""
    target = BUILDS_DIR / build_name
    if not (target / "bin" / "ros2").exists():
        print(f"Error: Build '{build_name}' not found", file=sys.stderr)
        return 1

    active = BUILDS_DIR / "active"
    if active.exists() or active.is_symlink():
        active.unlink()
    active.symlink_to(target)

    print(f"active -> {build_name}", file=sys.stderr)
    return 0


@dataclass
class EnvVar:
    """Definition of a manageable environment variable."""
    name: str                                    # Short name (e.g., "comp")
    env_var: Optional[str]                       # Full env var name (e.g., "COMP"), None for non-env vars
    config_file: Optional[str]                   # Config filename (e.g., "comp")
    description: str                             # Help text
    value_shortcuts: Dict[str, str] = field(default_factory=dict)  # Optional shortcuts
    getter: Optional[Callable[[], str]] = None   # Custom getter (for non-env vars like build)
    setter: Optional[Callable[[str], int]] = None  # Custom setter (for non-env vars like build)


# Registry - add new vars here
ENV_VARS: Dict[str, EnvVar] = {
    "comp": EnvVar(
        name="comp",
        env_var="COMP",
        config_file="comp",
        description="Competition mode (URC, ARCh)",
    ),
    "rmw": EnvVar(
        name="rmw",
        env_var="RMW_IMPLEMENTATION",
        config_file="rmw",
        description="ROS2 middleware implementation",
        value_shortcuts={
            "cyclone": "rmw_cyclonedds_cpp",
            "fastrtps": "rmw_fastrtps_cpp",
            "fast": "rmw_fastrtps_cpp",
        },
    ),
    "domain": EnvVar(
        name="domain",
        env_var="ROS_DOMAIN_ID",
        config_file="domain",
        description="ROS domain ID for network isolation",
    ),
    "dds": EnvVar(
        name="dds",
        env_var="CYCLONEDDS_URI",
        config_file="dds",
        description="CycloneDDS config file path",
    ),
    "build": EnvVar(
        name="build",
        env_var=None,  # Not an env var
        config_file=None,  # Uses symlink instead
        description="Active build (symlink)",
        getter=get_active_build,
        setter=set_active_build,
    ),
}


class EnvCommand(Command):
    """Implements 'nova env' command with subcommands."""

    @staticmethod
    def add_parser(subparsers):
        """Add env subcommand parser"""
        parser = subparsers.add_parser(
            'env',
            help='Manage environment configuration',
            description='Manage Nova environment variables and build configuration'
        )
        env_subparsers = parser.add_subparsers(dest='env_action')

        # nova env status
        env_subparsers.add_parser('status', help='Show all env vars')

        # nova env set <var> <value>
        set_parser = env_subparsers.add_parser('set', help='Set an env var')
        set_parser.add_argument('var', choices=list(ENV_VARS.keys()), help='Variable to set')
        set_parser.add_argument('value', help='Value to set')

        # nova env get <var>
        get_parser = env_subparsers.add_parser('get', help='Get an env var')
        get_parser.add_argument('var', choices=list(ENV_VARS.keys()), help='Variable to get')

        # nova env list
        env_subparsers.add_parser('list', help='List available vars')

        return parser

    @staticmethod
    def execute(args):
        """Execute env command"""
        if args.env_action is None or args.env_action == 'status':
            return EnvCommand._status()
        elif args.env_action == 'set':
            return EnvCommand._set(args.var, args.value)
        elif args.env_action == 'get':
            return EnvCommand._get(args.var)
        elif args.env_action == 'list':
            return EnvCommand._list()
        return 0

    @staticmethod
    def _status() -> int:
        """Show all environment variables and their current values."""
        for var_name, var in ENV_VARS.items():
            value = EnvCommand._get_value(var)
            if var_name == "build" and value != "(not set)":
                print(f"{var_name}:   active -> {value}")
            else:
                print(f"{var_name}:   {value}")
        return 0

    @staticmethod
    def _set(var_name: str, value: str) -> int:
        """Set an environment variable."""
        var = ENV_VARS[var_name]

        # Apply value shortcut if exists
        if var.value_shortcuts and value in var.value_shortcuts:
            value = var.value_shortcuts[value]

        if var.setter:
            # Custom setter (e.g., build)
            return var.setter(value)
        else:
            # Standard env var
            CONFIG_DIR.mkdir(parents=True, exist_ok=True)
            config_path = CONFIG_DIR / var.config_file
            config_path.write_text(f"export {var.env_var}={value}\n")

            # Output for eval
            print(f"export {var.env_var}={value}")

            # Status to stderr
            print(f"Set {var.env_var}={value} and wrote to {config_path}", file=sys.stderr)

        return 0

    @staticmethod
    def _get(var_name: str) -> int:
        """Get an environment variable's current value."""
        var = ENV_VARS[var_name]
        value = EnvCommand._get_value(var)
        print(value)
        return 0

    @staticmethod
    def _list() -> int:
        """List all available environment variables."""
        # Find max name length for alignment
        max_len = max(len(var.name) for var in ENV_VARS.values())
        for var in ENV_VARS.values():
            line = f"{var.name:<{max_len}}  {var.description}"
            if var.value_shortcuts:
                shortcuts = ", ".join(var.value_shortcuts.keys())
                line += f" (shortcuts: {shortcuts})"
            print(line)
        return 0

    @staticmethod
    def _get_value(var: EnvVar) -> str:
        """Get the current value of an environment variable."""
        if var.getter:
            return var.getter()

        # Check config file first
        if var.config_file:
            config_path = CONFIG_DIR / var.config_file
            if config_path.exists():
                content = config_path.read_text().strip()
                # Parse "export VAR=VALUE" format
                if content.startswith("export ") and "=" in content:
                    return content.split("=", 1)[1]

        # Fall back to environment variable
        import os
        if var.env_var:
            env_value = os.environ.get(var.env_var)
            if env_value:
                return env_value

        return "(not set)"
