import time

import jcan
import jcan.testing
import pytest
import rclpy
from nova_pytest_framework.ros2_helpers import (
    ServiceInteraction,
    TopicInteraction,
    spin_node_on_timer,
)
from science_interfaces.msg import ThermalData
from science_interfaces.srv import ThermalCommand

from ..arc.kiln import main as kiln_node

# CAN bus name and heater CAN ID's as configured in arc/kiln.py's `with_jcan()` /
# `with_hardware(...)` calls - kept in sync with that file.
CAN_BUS = "can1"
LEFT_HEATER_CAN_ID = 0x0C1
RIGHT_HEATER_CAN_ID = 0x0D2
THERMAL_COMMAND_SERVICE = "/science/thermal_command"
THERMAL_DATA_TOPIC = "/science/thermal_data"


@pytest.fixture()
def setup_common(setup_can, setup_ros2_sut, setup_ros2_tester):
    """Setup common factories and functions for use during tests"""
    bus = setup_can(CAN_BUS)

    sut = setup_ros2_sut("kiln", kiln_node)

    thermal_data = []

    def handle_thermal_data(msg: ThermalData):
        thermal_data.append(msg)

    tester = setup_ros2_tester(
        "kiln_tester",
        services=[
            {
                "service": THERMAL_COMMAND_SERVICE,
                "srv_type": ThermalCommand,
                "interaction": ServiceInteraction.CLIENT,
            }
        ],
        topics=[
            {
                "topic": THERMAL_DATA_TOPIC,
                "msg_type": ThermalData,
                "interaction": TopicInteraction.SUBSCRIBER,
                "callback": handle_thermal_data,
            }
        ],
    )

    yield bus, sut, tester, thermal_data


def test_kiln_temp(logger, setup_common):
    """
    Basic test for service call of kiln with check for existance of can messages on correct IDs + ros topic messages
    Procedure is the following:
    - Tester sends ThermalCommand containing {"state": true; "target": 100}
    - Kiln node should reply with a positive service response
    - Then CAN bus should have a message for each heater
    - Then after a pause there should be thermal data on the ros topic
    """
    _, _, tester, thermal_data = setup_common

    request = {"state": True, "target": 200}
    future = tester.send_request(THERMAL_COMMAND_SERVICE, request)
    rclpy.spin_until_future_complete(tester, future)
    response = future.result()

    assert response.success

    time.sleep(0.2)

    # Monitor can bus to determine that CAN data was sent
    sent_frames = jcan.testing.get_sent_frames(CAN_BUS)
    sent_ids = {frame.id for frame in sent_frames}
    logger.info(sent_frames)
    assert LEFT_HEATER_CAN_ID in sent_ids and RIGHT_HEATER_CAN_ID in sent_ids

    # Allow one second to collect ros topic messages
    spin_node_on_timer(tester, 1)

    # Check that there is thermal data on ros topic
    logger.info(thermal_data)
    assert len(thermal_data) > 1
