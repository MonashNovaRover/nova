import threading
from collections.abc import Callable, Generator
from enum import Enum
from typing import NotRequired, Protocol, TypedDict

import pytest
import rclpy
from rclpy.executors import SingleThreadedExecutor
from rclpy.node import Node
from rclpy.qos import QoSProfile, qos_profile_default, qos_profile_services_default
from rclpy.task import Future


class TopicInteraction(Enum):
    SUBSCRIBER = 0  # Listens for messages
    PUBLISHER = 1  # Sends messages
    BOTH = 2  # Can do both on this topic


class TopicElement(TypedDict):
    topic: str
    msg_type: any
    interaction: TopicInteraction
    callback: NotRequired[Callable]  # callback is not none if interaction is subscriber
    qos: NotRequired[QoSProfile]  # defaults to qos_profile_services_default


class ServiceInteraction(Enum):
    SERVER = 0  # Listens for requests
    CLIENT = 1  # Sends requests
    BOTH = 2  # Can do both on this service


class ServiceElement(TypedDict):
    service: str
    srv_type: any
    interaction: ServiceInteraction
    callback: NotRequired[Callable]  # callback is not none if interaction is server
    qos: NotRequired[QoSProfile]  # defaults to qos_profile_services_default


class TesterNode(Node):
    """Lightweight ROS 2 tester node used by pytest fixtures.

    The helper can create service clients and topic subscriptions for a test,
    wait for services to appear, and send asynchronous service requests.
    """

    def __init__(
        self,
        node_name: str,
        services: list[ServiceElement] | None = None,
        topics: list[TopicElement] | None = None,
    ):
        super().__init__(node_name)

        if services:
            self.node_servers = {}
            self.node_clients = {}
            for service in services:
                if "qos" not in service:
                    service["qos"] = qos_profile_services_default

                if service["interaction"] in [
                    ServiceInteraction.SERVER,
                    ServiceInteraction.BOTH,
                ]:
                    assert service["callback"] is not None
                    if service["qos"] is None:
                        service["qos"] = qos_profile_services_default
                    self.node_servers[service["service"]] = self.create_service(
                        service["srv_type"],
                        service["service"],
                        service["callback"],
                        qos_profile=service["qos"],
                    )
                if service["interaction"] in [
                    ServiceInteraction.CLIENT,
                    ServiceInteraction.BOTH,
                ]:
                    self.node_clients[service["service"]] = self.create_client(
                        service["srv_type"],
                        service["service"],
                        qos_profile=service["qos"],
                    )
                    if not self.node_clients[service["service"]].wait_for_service(
                        timeout_sec=5.0
                    ):
                        raise TimeoutError(
                            f'Service "{service["service"]}" not available.'
                        )

        if topics:
            self.node_publishers = {}
            self.node_subscriptions = {}
            for topic in topics:
                if "qos" not in topic:
                    topic["qos"] = qos_profile_default

                if topic["interaction"] in [
                    TopicInteraction.PUBLISHER,
                    ServiceInteraction.BOTH,
                ]:
                    self.node_publishers[topic["topic"]] = self.create_publisher(
                        topic["msg_type"], topic["topic"], topic["qos"]
                    )
                if topic["interaction"] in [
                    TopicInteraction.SUBSCRIBER,
                    ServiceInteraction.BOTH,
                ]:
                    assert topic["callback"] is not None
                    self.node_subscriptions[topic["topic"]] = self.create_subscription(
                        topic["msg_type"],
                        topic["topic"],
                        topic["callback"],
                        topic["qos"],
                    )

    def send_request(self, service: str, payload: dict = {}):
        """Build and send an async service request for a named service."""
        req = self.node_clients[service].srv_type.Request(**payload)
        return self.node_clients[service].call_async(req)

    def publish(self, topic: str, msg: any):
        """Publish on the topic"""
        if topic in self.node_publishers:
            self.node_publishers[topic].publish(msg)
        else:
            raise ValueError(f"Topic '{topic}' not found in the node's subscriptions.")

    # TODO: Implement topic callback handler that runs in background
    #       Right now node must be spun manually for listening to topic


class Ros2TesterFactory(Protocol):
    def __call__(
        self,
        node_name: str,
        services: list[ServiceElement] | None = None,
        topics: list[TopicElement] | None = None,
    ) -> TesterNode: ...


class Ros2SutFactory(Protocol):
    def __call__(
        self,
        node_name: str,
        node_spawner: Callable[[Node], Node],
        *node_args: object,
        **node_kwargs: object,
    ) -> Node: ...


def spin_node_on_timer(node: Node, seconds: int):
    """
    Spins node for x seconds.
    Since it uses spin until future complete there is no latency.
    This is a blocking call.
    """
    future = Future()
    node.create_timer(seconds, lambda: future.set_result(True))
    rclpy.spin_until_future_complete(node, future)


@pytest.fixture(scope="session")
def initalise_ros2():
    """
    Fixture to initalise ros2 for the session,
    shared for all fixtures which use ros2
    """
    rclpy.init()
    yield
    rclpy.shutdown()


@pytest.fixture()
def setup_ros2_tester(initalise_ros2) -> Generator[Ros2TesterFactory, None, None]:
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

    yield _make_tester
    for tester in created_testers:
        tester.destroy_node()


@pytest.fixture()
def ros2_sut_executor(initalise_ros2):
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
) -> Generator[Ros2SutFactory, None, None]:
    """
    Fixture factory that constructs a (System Under Test) node.

    The provided spawner is expected to return a ControllerManager configured
    for the node, then `spin(auto_run_rclpy=False)` wires up the timers and
    services without blocking the test process.
    """
    nodes: list[Node] = []

    def _make_sut_node(
        node_spawner: Callable[[Node], Node],
        *node_args: object,
        **node_kwargs: object,
    ) -> Node:
        node_spawner(*node_args, **node_kwargs).spin(
            auto_run_rclpy=False
        )  # sets up timers/services, doesn't block
        ros2_sut_executor.add_node(node)
        nodes.append(node)
        return node

    yield _make_sut_node
    for node in nodes:
        node.destroy_node()


@pytest.fixture()
def setup_python_control2_sut(
    ros2_sut_executor: SingleThreadedExecutor,
) -> Generator[Ros2SutFactory, None, None]:
    nodes: list[Node] = []
    def _make_sut_node(node_spawner: Callable[[Node], Node], *node_args: object, **node_kwargs: object) -> Node:
        node = Node(*node_args, **node_kwargs)
        node_spawner(node).spin(
            auto_run_rclpy=False
        )  # sets up timers/services, doesn't block
        ros2_sut_executor.add_node(node)
        nodes.append(node)
        return node
    
    yield _make_sut_node
    for node in nodes:
        node.destroy_node()