#!/usr/bin/env python3
from enum import Enum

class Card(Enum):
    """Enum for the different cards on the CAN bus"""
    CMD = "CMD"
    JONO = "JONO"
    STEPPER_PCB = "STEPPER_PCB"
    TOGGLE = "TOGGLE"