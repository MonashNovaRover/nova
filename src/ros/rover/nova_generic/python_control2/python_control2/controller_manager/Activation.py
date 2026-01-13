from rclpy.node import Node
from teleop_python_utils import Button

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

        self.node.get_logger().info(f"{self.node.get_name()} is {"ACTIVE" if self.active else "INACTIVE"}")

    def activate(self):
        """ Activates the system """
        self.active = True
        self.node.get_logger().info(f"{self.node.get_name()} ACTIVATED")

    def deactivate(self):
        """ Deactivates the system """
        self.active = False
        self.node.get_logger().info(f"{self.node.get_name()} DEACTIVATED")

    def is_active(self):
        return self.active

    def __bool__(self):
        return self.active
