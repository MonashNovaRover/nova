#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This script executes commands on the CMDs that are
connected to the rover. It can change the ID and the
data that is sent.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:     electronics
AUTHOR(S):   Harrison Verrios
CREATION:    21/05/2022
EDITED:      21/05/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Available services:
    - /electronics/cmd_service 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

# Import the relevant ROS nodes
from core.srv import CMDCommand
import rclpy, time
from rclpy.node import Node

# Import the relevant CAN nodes
from coms_utils.can_interface import CANTransmitter


# The main class behind this transmitter. It will handle a
# ROS message incoming from a service and communicate over
# a CAN network.
class CMDService (Node):

    # The CAN networks
    can_network_0: CANTransmitter = None
    can_network_1: CANTransmitter = None

    # The endian of the data transmitted
    # Could be 'big' or 'little'
    endian = "big"

    # The constructor sets up any variables required on start-up
    def __init__(self):
        super().__init__('cmd_service')

    
    # Retrieves a CAN network based on an ID. If the network
    # does not yet exist, it will attempt to create the 
    # transmitter.
    def get_can (self, network: int) -> CANTransmitter:

        # Ensure the bounds
        if network < 0 or network > 1:
            raise Exception("Invalid CAN network entered: %d" % network)
        
        # Get the network
        transmitter: CANTransmitter = self.can_network_0 if network == 0 else self.can_network_1

        # Check if it is null
        if transmitter == None:

            # Create the network
            if network == 0:
                self.can_network_0 = CANTransmitter(channel=("can%d" % network), arbitration_id=0x000, bitrate=200000)
            else:
                self.can_network_1 = CANTransmitter(channel=("can%d" % network), arbitration_id=0x000, bitrate=200000)

        # Return the network
        return self.can_network_0 if network == 0 else self.can_network_1

    
    # Constructs and ID based on some values that are strings
    # These strings must be HEX values.
    def get_id (self, device: str, command: int) -> int:

        # Ensure the command is in range
        if command < 0 or command > 7:
            raise Exception("Invalid command enetered: %d" % command)
        
        # Ensure the device is two characters long
        if len(device) < 2:
            raise Exception("Device ID is too short: %s" % device)
        device = device[0:2]  

        # Append the commands together to create the HEX ID
        hex_id: str = "0x%s%d" % (device, command)

        # Return the final ID
        return int(hex_id, 16)

    
    # Executes a command with some data
    def send_data (self, can: CANTransmitter, id: int, data: str = "") -> bool:

        # Updates the arbitration ID
        can.arbitration_id = id

        # Ensures the data is of the right length
        if len(data) % 2 == 1:
            raise Exception("Invalid length of HEX data: %s" % data)

        # If data is missing, use zeroes
        if data == "":
            data = "00"

        # Converts the data into some bytes
        int_value: int = int(data, 16)
        data_bytes: bytes = int.to_bytes(int_value, int(len(data) / 2), self.endian)

        # Transmits the data and return a success
        return can.transmit(data_bytes)

    
    # Executes a command from ROS
    def execute (self) -> None:

        # Attempts to catch any exceptions
        try:

            # Retrieves the CAN network
            can = self.get_can(0)

            # Gets the ID
            id = self.get_id("01", 2)

            # Executes the command
            self.send_data(can, id, "007C8C9A")
        
        # Catches the exception
        except Exception as e:
            print("ERROR: %s" % e)



# The main function sets up ROS and the class
def main (args=None):

    # Create the service
    rclpy.init(args = args)
    service = CMDService()

    # TODO REMOVE THIS 
    service.execute()
    rclpy.spin(service)

    # Clean up when complete
    service.destroy_node()
    rclpy.shutdown()


# Called when this script is executed
if __name__ == "__main__":
    main()