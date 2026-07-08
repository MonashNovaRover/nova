"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Bash completion support for nova CLI. Provides completers for
packages, launch files, executables, and scripts using filesystem
lookups against the active build.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        nova_cli
AUTHOR(S):      Felicity Matthews
CREATION:       26/01/26
EDITED:         26/07/09
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
from pathlib import Path


def _get_build_path():
    """Get the build path from environment or default to active"""
    return Path.home() / "Builds" / "active"


def complete_packages(prefix, parsed_args, **kwargs):
    """Complete package names using filesystem"""
    share_dir = _get_build_path() / "share"
    if not share_dir.exists():
        return []

    try:
        packages = [d.name for d in share_dir.iterdir() if d.is_dir()]
        return [p for p in packages if p.startswith(prefix)]
    except OSError:
        return []


def complete_packages_smart(prefix, parsed_args, **kwargs):
    """Complete package names for launch command (shows base names too)"""
    share_dir = _get_build_path() / "share"
    if not share_dir.exists():
        return []

    try:
        packages = [d.name for d in share_dir.iterdir() if d.is_dir()]
    except OSError:
        return []

    results = []
    for pkg in packages:
        if pkg.startswith(prefix):
            results.append(pkg)
        if pkg.endswith('_bringup'):
            base = pkg[:-8]
            if base.startswith(prefix):
                results.append(base)
    return results


def complete_launch_files(prefix, parsed_args, **kwargs):
    """Complete launch file names using filesystem"""
    if not hasattr(parsed_args, 'package') or not parsed_args.package:
        return []

    package = parsed_args.package
    if package == "teleop":
        subsystems = ['drive', 'arm', 'science', 'ec']
        return [s for s in subsystems if s.startswith(prefix)]

    # Try _bringup suffix first
    test_pkg = package if package.endswith('_bringup') else f"{package}_bringup"
    launch_dir = _get_build_path() / "share" / test_pkg / "launch"

    if not launch_dir.exists():
        # Try without _bringup
        launch_dir = _get_build_path() / "share" / package / "launch"

    if not launch_dir.exists():
        return []

    try:
        launch_files = []
        for f in launch_dir.glob("*.launch.py"):
            base_name = f.stem.replace('.launch', '')
            if base_name.startswith(prefix):
                launch_files.append(base_name)
        return launch_files
    except OSError:
        return []


def complete_executables(prefix, parsed_args, **kwargs):
    """Complete executable names using filesystem"""
    if not hasattr(parsed_args, 'package') or not parsed_args.package:
        return []

    package = parsed_args.package
    lib_dir = _get_build_path() / "lib" / package

    if not lib_dir.exists():
        return []

    try:
        executables = []
        for f in lib_dir.iterdir():
            if f.is_file():
                name = f.name
                if name.endswith('.py'):
                    base_name = name[:-3]
                    if base_name.startswith(prefix) or name.startswith(prefix):
                        executables.append(base_name)
                elif name.startswith(prefix):
                    executables.append(name)
        return executables
    except OSError:
        return []


def complete_scripts(prefix, parsed_args, **kwargs):
    """Complete script names from launch directory"""
    launch_dir = _get_build_path() / "launch"
    if not launch_dir.exists():
        return []

    try:
        scripts = []
        for f in launch_dir.iterdir():
            if f.is_file() and f.name.startswith(prefix):
                scripts.append(f.name)
        return sorted(scripts)
    except OSError:
        return []
