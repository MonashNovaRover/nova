#!/usr/bin/env python3

import os
import sys
from pathlib import Path

# Make `import jcan` resolve to the pure-python mock (src/other/mock/JCAN) instead of
# the real (compiled) jcan_python library, so tests can run without real/virtual CAN
# hardware. This must happen before anything that transitively imports `jcan` (e.g.
# python_control2 hardware interfaces, used by nodes like kiln.py) gets imported.
_MOCK_JCAN_DIR = Path("../../../../../../other/mock/JCAN")
sys.path.insert(0, str(_MOCK_JCAN_DIR))
os.environ["PYTHONPATH"] = os.pathsep.join(
    [str(_MOCK_JCAN_DIR), os.environ.get("PYTHONPATH", "")]
)

import rclpy
import json
import pytest

from rclpy.node import Node
import jcan.testing

from typing import TypedDict

class TopicList(TypedDict):
    topic: str
    msg_type: any
    callback: any
class ServiceList(TypedDict):
    service: str
    srv_type: any

# Create client node to send requests and listen to topics
class ClientNode(Node):
    def __init__(self, services: list[ServiceList]=None, topics: list[TopicList]=None):
        super().__init__('test_node')

        if not services:
            self.services = {}
            for service in services:
                self.services[service['service']] = self.create_client(service['srv_type'], service['service'])
                if not self.services[service['service']].wait_for_service(timeout_sec=5.0):
                    raise TimeoutError(f'Service "{service["service"]}" not available, Waiting again...')

        if not topics:
            self.topics = {}
            for topic in topics:
                self.topics[topic['topic']] = self.create_subscription(topic['msg_type'], topic['topic'], topic['callback'], 10)
    
    # Sends the request
    def send_request(self, service: str, payload: dict = None):
        req = self.services[service].srv_type.Request()
        req.command = json.dumps(payload)
        return self.services[service].call_async(req)

    def publish(self, topic: str, msg):
        if topic in self.topics:
            self.topics[topic].publish(msg)
        else:
            raise ValueError(f"Topic '{topic}' not found in the node's subscriptions.")


@pytest.fixture(scope="function")
def setup_client_node(services: list[ServiceList]=None, topics: list[TopicList]=None):
    """
    Fixture to provide a setup client node for testing.
    """
    rclpy.init()
    client = ClientNode(services=services, topics=topics)
    yield client
    client.destroy_node()
    rclpy.shutdown()


@pytest.fixture(autouse=True)
def _reset_mock_jcan():
    """
    Ensures virtual CAN networks from the mock jcan library (src/other/mock/JCAN)
    don't leak state (e.g. sent frames, open buses) from one test into the next.
    """
    yield
    jcan.testing.reset_all_networks()
