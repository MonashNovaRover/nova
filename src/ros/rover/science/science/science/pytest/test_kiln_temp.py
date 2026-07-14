#!/usr/bin/env python3

import time

import jcan
import jcan.testing
import pytest
import rclpy
from science_interfaces.srv import ThermalCommand

from ..arc.kiln import main as kiln_node

SERVICE = "/science/thermal_command"

# CAN bus name and heater CAN ID's as configured in arc/kiln.py's `with_jcan()` /
# `with_hardware(...)` calls - kept in sync with that file.
CAN_BUS = "can1"
LEFT_HEATER_CAN_ID = 0x0C1
RIGHT_HEATER_CAN_ID = 0x0D2


# Main function for setting up the ROS node
@pytest.mark.manual
def test_kiln_temp(logger, setup_tester, setup_sut):
    setup_sut("kiln", kiln_node)
    tester = setup_tester(
        services=[{"service": SERVICE, "srv_type": ThermalCommand}], topics=None
    )

    # Observe the same (mocked) virtual CAN network kiln.py's node sends on, so we
    # can check it actually commanded the heaters over CAN.
    bus = jcan.Bus()
    bus.open(CAN_BUS)

    request = {"state": True, "target": 0}
    future = tester.send_request(SERVICE, request)
    rclpy.spin_until_future_complete(tester, future)
    response = future.result()

    assert response.success

    time.sleep(0.2)

    # Monitor mock JCAN can bus to determine correct CAN data is sent
    sent_frames = jcan.testing.get_sent_frames(CAN_BUS)
    sent_ids = {frame.id for frame in sent_frames}
    logger.info(sent_frames)
    assert LEFT_HEATER_CAN_ID in sent_ids or RIGHT_HEATER_CAN_ID in sent_ids
