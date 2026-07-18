from collections.abc import Callable
from enum import Enum
from typing import NotRequired, TypedDict

import rclpy
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


def spin_node_on_timer(node: Node, seconds: int):
    """
    Spins node for x seconds.
    Since it uses spin until future complete there is no latency.
    This is a blocking call.
    """
    future = Future()
    node.create_timer(seconds, lambda: future.set_result(True))
    rclpy.spin_until_future_complete(node, future)
