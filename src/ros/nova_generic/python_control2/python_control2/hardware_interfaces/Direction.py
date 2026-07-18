#!/usr/bin/env python3

from enum import Enum


class Direction(Enum):
    """Enum for the different directions of the motors"""
    POSITIVE = 1
    NEGATIVE = -1