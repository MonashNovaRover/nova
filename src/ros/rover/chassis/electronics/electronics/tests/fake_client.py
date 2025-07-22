#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
from std_srvs.srv import Trigger


class LEDClient(Node):
    def __init__(self):
        super().__init__("LED_client")
        self.client = self.create_client(Trigger, "/autonomous/LED")

    def send(self):
        msg = Trigger.Request()
        self.future = self.client.call_async(msg)

if __name__ == "__main__":
    rclpy.init()
    client = LEDClient()
    while True:
        input("Finished autonomous:")
        client.send()
    rclpy.shutdown()

