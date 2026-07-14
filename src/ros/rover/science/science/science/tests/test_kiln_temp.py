#!/usr/bin/env python3

import rclpy
import json
import pytest

import jcan
import jcan.testing


from science_interfaces.srv import ThermalCommand
SERVICE = '/science/thermal_command'

# CAN bus name and heater CAN ID's as configured in arc/kiln.py's `with_jcan()` /
# `with_hardware(...)` calls - kept in sync with that file.
CAN_BUS = 'can1'
LEFT_HEATER_CAN_ID = 0x0C1
RIGHT_HEATER_CAN_ID = 0x0D2


# Main function for setting up the ROS node    
def test_kiln_temp(setup_client_node):
    client = setup_client_node(
        services=[{'service': SERVICE, 'srv_type': ThermalCommand}], 
        topics=None
    )

    # Observe the same (mocked) virtual CAN network kiln.py's node sends on, so we
    # can check it actually commanded the heaters over CAN.
    bus = jcan.Bus()
    bus.open(CAN_BUS)

    request = {'state': True, 'target': 0}
    future = client.send_request(SERVICE, request)
    rclpy.spin_until_future_complete(client, future)
    response = future.result()

    assert response.success

    # Monitor mock JCAN can bus to determine correct CAN data is sent
    sent_frames = jcan.testing.get_sent_frames(CAN_BUS)
    sent_ids = {frame.id for frame in sent_frames}
    assert LEFT_HEATER_CAN_ID in sent_ids or RIGHT_HEATER_CAN_ID in sent_ids
