#!/usr/bin/env python3

import logging
import threading
from collections.abc import Callable
from typing import TypedDict

import jcan
import jcan.testing
import pytest
import rclpy
from python_control2.controller_manager import ControllerManager
from rclpy.executors import SingleThreadedExecutor
from rclpy.node import Node


class TopicList(TypedDict):
    topic: str
    msg_type: any
    callback: any


class ServiceList(TypedDict):
    service: str
    srv_type: any


class TesterNode(Node):
    """Lightweight ROS 2 tester node used by pytest fixtures.

    The helper can create service clients and topic subscriptions for a test,
    wait for services to appear, and send asynchronous service requests.
    """

    def __init__(
        self,
        node_name: str,
        services: list[ServiceList] | None = None,
        topics: list[TopicList] = None,
    ):
        super().__init__(node_name)

        if services:
            self.node_services = {}
            for service in services:
                self.node_services[service["service"]] = self.create_client(
                    service["srv_type"], service["service"]
                )
                if not self.node_services[service["service"]].wait_for_service(
                    timeout_sec=5.0
                ):
                    raise TimeoutError(f'Service "{service["service"]}" not available.')

        if topics:
            self.node_topics = {}
            for topic in topics:
                self.node_topics[topic["topic"]] = self.create_subscription(
                    topic["msg_type"], topic["topic"], topic["callback"], 10
                )

    def send_request(self, service: str, payload: dict = {}):
        """Build and send an async service request for a named service."""
        req = self.node_services[service].srv_type.Request(**payload)
        return self.node_services[service].call_async(req)

    def publish(self, topic: str, msg):
        if topic in self.node_topics:
            self.node_topics[topic].publish(msg)
        else:
            raise ValueError(f"Topic '{topic}' not found in the node's subscriptions.")


@pytest.fixture(scope="session")
def logger():
    return logging.getLogger(__name__)


@pytest.fixture()
def setup_tester():
    """
    Fixture factory that creates and tracks client nodes for each test.

    rclpy is initialized once for the test, then each created client node is
    destroyed before rclpy is shut down at the end of the fixture.
    """
    created_testers = []

    def _make_tester(
        node_name: str,
        services: list[ServiceList] = None,
        topics: list[TopicList] = None,
    ):
        tester = TesterNode(node_name, services=services, topics=topics)
        created_testers.append(tester)
        return tester

    rclpy.init()
    yield _make_tester
    for tester in created_testers:
        tester.destroy_node()
    rclpy.shutdown()


@pytest.fixture()
def sut_executor():
    """
    Run a SingleThreadedExecutor in a background thread for the SUT node.

    The executor is spun separately so the node under test can process timers,
    services, and subscriptions while the test thread sends requests.
    """
    executor = SingleThreadedExecutor()
    stop = threading.Event()
    thread = threading.Thread(
        target=lambda: [
            executor.spin_once(timeout_sec=0.05)
            for _ in iter(lambda: not stop.is_set(), False)
        ],
        daemon=True,
    )
    thread.start()
    yield executor
    stop.set()
    thread.join(timeout=2.0)


@pytest.fixture()
def setup_sut(sut_executor):
    """
    Fixture factory that constructs a (System Under Test) node.

    The provided spawner is expected to return a ControllerManager configured
    for the node, then `spin(auto_run_rclpy=False)` wires up the timers and
    services without blocking the test process.
    """
    nodes = []

    def _make_sut_node(
        node_name: str, node_spawner: Callable[[Node], ControllerManager]
    ):
        node = Node(node_name)
        node_spawner(node).spin(
            auto_run_rclpy=False
        )  # sets up timers/services, doesn't block
        sut_executor.add_node(node)
        nodes.append(node)
        return node

    yield _make_sut_node
    for node in nodes:
        node.destroy_node()


@pytest.fixture(autouse=True)
def reset_mock_jcan():
    """
    Ensures virtual CAN networks from the mock jcan library (src/other/mock/JCAN)
    don't leak state (e.g. sent frames, open buses) from one test into the next.
    """
    yield
    jcan.testing.reset_all_networks()
