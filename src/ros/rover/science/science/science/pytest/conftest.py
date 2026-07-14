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


# Create client node to send requests and listen to topics
class ClientNode(Node):
    def __init__(
        self, services: list[ServiceList] | None = None, topics: list[TopicList] = None
    ):
        super().__init__("test_node")

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

    # Sends the request
    def send_request(self, service: str, payload: dict = {}):
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
    Fixture factory to setup a tester node.
    """
    created_clients = []

    def _make_client(
        services: list[ServiceList] = None, topics: list[TopicList] = None
    ):
        client = ClientNode(services=services, topics=topics)
        created_clients.append(client)
        return client

    rclpy.init()
    yield _make_client
    for client in created_clients:
        client.destroy_node()
    rclpy.shutdown()


@pytest.fixture()
def sut_executor():
    """
    Creates an executor for a SUT node
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
    Fixture factory to create a SUT (System Under Test) node
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
