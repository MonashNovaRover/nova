import logging
import threading
from collections.abc import Callable, Generator
from typing import Protocol

import jcan
import jcan.testing
import pytest
import rclpy
from rclpy.executors import SingleThreadedExecutor
from rclpy.node import Node

from .ros2_helpers import ServiceElement, TesterNode, TopicElement


class TesterFactory(Protocol):
    def __call__(
        self,
        node_name: str,
        services: list[ServiceElement] | None = None,
        topics: list[TopicElement] | None = None,
    ) -> TesterNode: ...


class SutFactory(Protocol):
    def __call__(
        self,
        node_name: str,
        node_spawner: Callable[[Node], Node],
    ) -> Node: ...


@pytest.fixture(scope="session")
def logger():
    return logging.getLogger(__name__)


@pytest.fixture()
def setup_ros2_tester() -> Generator[TesterFactory, None, None]:
    """
    Fixture factory that creates and tracks client nodes for each test.

    rclpy is initialized once for the test, then each created client node is
    destroyed before rclpy is shut down at the end of the fixture.
    """
    created_testers: list[TesterNode] = []

    def _make_tester(
        node_name: str,
        services: list[ServiceElement] | None = None,
        topics: list[TopicElement] | None = None,
    ) -> TesterNode:
        tester = TesterNode(node_name, services=services, topics=topics)
        created_testers.append(tester)
        return tester

    rclpy.init()
    yield _make_tester
    for tester in created_testers:
        tester.destroy_node()
    rclpy.shutdown()


@pytest.fixture()
def ros2_sut_executor():
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
def setup_ros2_sut(
    ros2_sut_executor: SingleThreadedExecutor,
) -> Generator[SutFactory, None, None]:
    """
    Fixture factory that constructs a (System Under Test) node.

    The provided spawner is expected to return a ControllerManager configured
    for the node, then `spin(auto_run_rclpy=False)` wires up the timers and
    services without blocking the test process.
    """
    nodes: list[Node] = []

    def _make_sut_node(
        node_name: str, node_spawner: Callable[[Node], Node]
    ) -> Node:
        node = Node(node_name)
        node_spawner(node).spin(
            auto_run_rclpy=False
        )  # sets up timers/services, doesn't block
        ros2_sut_executor.add_node(node)
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
