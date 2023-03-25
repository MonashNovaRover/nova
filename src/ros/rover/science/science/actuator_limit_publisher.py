#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This file contains the ROS2 publisher code for the actuator limit switch.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ PACKAGE:     electronics
AUTHOR(S):   Niko Verrios
CREATION:    23/03/2023
EDITED:      23/03/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
import jcan

# import custom messages
from core.msg import ActuatorLimit


class ActuatorLimitPublisher(Node):

    def __init__(self):
        super().__init__("actuator_limit_publisher")


        # Print initialisation information
        self.get_logger().warning("\033[92;1mInitialising the Actuator Limit Publisher class.\033[0m")

        #publisher to publish the data from the kilns.
        self.publisher = self.create_publisher(ActuatorLimit, "/science/actuator_limit", 1)

        #declare parameters
        self.declare_parameter("canbus", "can1")

        #initialise the can bus
        self.bus = jcan.Bus()

        # Set filter IDs and callbacks.
        self.bus.set_id_filter([0x4C2])
        self.bus.add_callback(0x4C2, self.get_callback())

        #create timers
        self.can_spin_timer = self.create_timer(0.05, self.bus.spin)
        self.publish_data_timer = self.create_timer(0.1, self.publish_data)

        # polling times set
        self.top_limit = False
        self.bottom_limit = False

        self.data_changed = False

        #open the can bus
        self.bus.open(self.get_parameter("canbus").value)


    def get_callback(self):
        """
        Returns a callback function for the load cell
        :return:
        """
        def callback(frame):
            # Data returned is a byte list. First byte is the kiln ID, other bytes is the remaining data.
            try:
                id = int(frame.data[0])
                data = int.from_bytes(bytes(frame.data[1:]), "big", signed=False)
                if id == 1:
                    if data == 0 and self.bottom_limit:
                        self.bottom_limit = False
                        self.data_changed = True
                    elif data == 1 and not self.bottom_limit:
                        self.bottom_limit = True
                        self.data_changed = True
                elif id == 2:
                    if data == 0 and self.top_limit:
                        self.top_limit = False
                        self.data_changed = True
                    elif data == 1 and not self.top_limit:
                        self.top_limit = True
                        self.data_changed = True
                else:
                    raise Exception("ID did not return '1' or '2'.")
                    

            except Exception as e:
                self.get_logger().error(f"\033[91;1mLimit switch hit an error: {e}\033[0m")
                
        return callback
    
    
    def publish_data(self):
        # Publish limit switch message data.
        if self.data_changed:
            msg = ActuatorLimit()
            msg.top = self.top_limit
            msg.bottom = self.bottom_limit
            self.publisher.publish(msg)
            self.get_logger().info(f"\033[92;1mPublished: Top: {msg.top}, Bottom: {msg.bottom}.\033[0m")
            self.data_changed = False



def main():
    rclpy.init()
    publisher_node = ActuatorLimitPublisher()
    rclpy.spin(publisher_node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()