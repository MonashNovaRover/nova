#!/usr/bin/env python3

import time

import jcan
import jcan.testing
import rclpy
from nova_pytest_framework.plugin import ServiceInteraction, TopicInteraction, spin_node_on_timer
from science_interfaces.msg import ThermalData
from science_interfaces.srv import ThermalCommand

from ..arc.kiln import can_bus, command_service, data_topic
from ..arc.kiln import main as kiln_node

# CAN bus name and heater CAN ID's as configured in arc/kiln.py's `with_jcan()` /
# `with_hardware(...)` calls - kept in sync with that file.
LEFT_HEATER_CAN_ID = 0x0C1
RIGHT_HEATER_CAN_ID = 0x0D2


def test_kiln_temp(logger, setup_tester, setup_sut):
    """
    Basic test for service call of kiln with check for existance of can messages on correct IDs + ros topic messages
    """
    setup_sut("kiln", kiln_node)
    thermal_data = []
    def handle_thermal_data(msg: ThermalData):
        thermal_data.append(msg)


    tester = setup_tester(
        "kiln_tester",
        services=[
            {
                "service": command_service,
                "srv_type": ThermalCommand,
                "interaction": ServiceInteraction.CLIENT,
            }
        ],
        topics=[
            {
                "topic": data_topic,
                "msg_type": ThermalData,
                "interaction": TopicInteraction.SUBSCRIBER,
                "callback": handle_thermal_data,
            }
        ],
    )

    # Observe the same (mocked) virtual CAN network kiln.py's node sends on, so we
    # can check it actually commanded the heaters over CAN.
    bus = jcan.Bus()
    bus.open(can_bus)

    request = {"state": True, "target": 100}
    future = tester.send_request(command_service, request)
    rclpy.spin_until_future_complete(tester, future)
    response = future.result()

    assert response.success

    time.sleep(0.2)

    # Monitor mock JCAN can bus to determine that CAN data was sent
    sent_frames = jcan.testing.get_sent_frames(can_bus)
    sent_ids = {frame.id for frame in sent_frames}
    logger.info(sent_frames)
    assert LEFT_HEATER_CAN_ID in sent_ids or RIGHT_HEATER_CAN_ID in sent_ids

    # Allow one second to collect ros topic messages
    spin_node_on_timer(tester, 1)

    # Check that there is thermal data on ros topic
    logger.info(thermal_data)
    assert len(thermal_data) > 1


