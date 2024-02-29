#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
Handles enacting commands received over ROS and feedback received through CAN
for the kiln.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: kiln_server
TOPICS:
    - /science/kiln_data                    [pub]
SERVICES:
    - /science/kiln_command              [server]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      Connor Macdougall
CREATION:       29/02/2024
EDITED:         30/02/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import rclpy
from rclpy.node import Node
import jcan

from core.msg import KilnData
from core.srv import KilnCommand