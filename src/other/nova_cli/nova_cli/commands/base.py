"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Abstract base class for nova CLI commands. Provides common interface
for parser registration and command execution.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        nova_cli
AUTHOR(S):      Felicity Matthews
CREATION:       06/07/2026
EDITED:         09/07/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
from abc import ABC, abstractmethod


class Command(ABC):
    """Base class for nova commands"""

    @staticmethod
    @abstractmethod
    def add_parser(subparsers):
        """Add this command to the argument parser"""
        pass

    @staticmethod
    @abstractmethod
    def execute(args):
        """Execute the command"""
        pass
