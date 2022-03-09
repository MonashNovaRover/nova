#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This file contains a convenience little wrapper on a ros service client to call
the rfid service and print out the responses. Its pretty jank but should
just be an intermediate testing thing to having a GUI
The script is designed to appear somewhat like a terminal which will accept
input commands to appropriately control and read data from the RFID tag

usage examples:
>>>read
<data read from rfid or error message>
>>>write <data to write to rfid card>
<response message>
>>>clear
<response message>
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:     electronics
AUTHOR(S):    Josh Cherubino
CREATION:    9/3/2022
EDITED:      9/3/2022 by Josh Cherubino
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
from core.srv import RFIDCommand
import rclpy
from rclpy.node import Node

class RFIDClient(Node):

    def __init__(self):
        super().__init__('rfid_client_terminal')
        self.cli = self.create_client(RFIDCommand, '/electronics/rfid_service')
        while not self.cli.wait_for_service(timeout_sec=1.0):
            print('Service not available')
        self.future = None

    def send_request(self):
        req = RFIDCommand.Request()
        # The first word is the command, and the second word is the data field
        # Accordingly, split on space allowing max of 1 split
        split_string = input('>>>').strip().split(' ', 1)
        # should always be at least 1 element
        req.command = split_string[0]
        if len(split_string == 2):
            req.data = split_string[1]

        self.future = self.cli.call_async(req)


def main(args=None):
    rclpy.init(args=args)

    rfid_client = RFIDClient()

    while rclpy.ok():
        if rfid_client.future is None:
            # then we should ask request
            rfid_client.send_request()
        rclpy.spin_once(rfid_client)
        if rfid_client.future.done():
            try:
                # get and print text data to user
                response = rfid_client.future.result()
                print(response.response)
                rfid_client.future = None # clear future to indicate new request can be made
            except Exception as e:
                print('Service call failed')

    rfid_client.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
