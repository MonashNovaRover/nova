from rclpy.node import Node
from teleop_python_utils import Button
from nova_interfaces.msg import ActiveNodeStatus

class Activation:
    """
    Class to represent whether a python control2 system is active or not

    Has a teleop button that when pressed activates the system and a
    pool of buttons that when pressed deactivates the system.
    """

    def __init__(self, active_button: Button, inactive_button_pool: list[Button], node: Node, start_active: bool=False):
        """
        :param active_button: Button that activates.
        :param inactive_button_pool: Buttons that deactivate.
        :param start_active: Whether to start active or not, defaults to False.
        """
        self.active = start_active
        self.node = node
        self.active_button = active_button
        self.inactive_button_pool = inactive_button_pool

        # Add button callbacks
        self.active_button.add_callback(self.activate)
        for but in inactive_button_pool:
            but.add_callback(self.deactivate)

        # Publisher for active controller telemetry
        self.active_node_publisher = self.node.create_publisher(ActiveNodeStatus, "/activated_nodes", 10)

        self.node.get_logger().info(f"{self.node.get_name()} is {"ACTIVE" if self.active else "INACTIVE"}")

    def publish_msg(self):
        """ Publishes message containing name and active status of controller node """
        # Creating active node msg data type
        msg = ActiveNodeStatus()

        msg.name = self.node.get_name()
        msg.active = self.is_active()
        msg.locked = False

        # Sending message over topic
        self.active_node_publisher.publish(msg)

    def activate(self):
        """ Activates the system """
        if not self.active:
            self.node.get_logger().info(f"{self.node.get_name()} ACTIVATED")
        self.active = True

    def deactivate(self):
        """ Deactivates the system """
        if self.active:
            self.node.get_logger().info(f"{self.node.get_name()} DEACTIVATED")
        self.active = False

    def is_active(self):
        return self.active

    def __bool__(self):
        return self.active
