#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This script is able to move the gimbal cameras,
which are controlled over CAN, around at move them
to particular locations on the screen.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: gimbal_service
SERVICES:
  - /electronics/gimbal_command  [GimbalCommand]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	electronics
AUTHOR(S):	Harrison Verrios
CREATION:	25/02/2022
EDITED:		25/02/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

# Include ROS packages
import rclpy
from rclpy.node import Node

# Import the gimbal service
from core.srv import GimbalCommand

# Import the CAN library
from coms_utils.can_interface import CANTransmitter



# Main Gimbal Service class
class GimbalService (Node):


    # Constructor sets up the service
    def __init__ (self):
    
        # Set up the node
        super().__init__("gimbal_service")

        # Print initialisation information
        print("Initialising the Gimbal Service class.")

        # Set up the CAN interface for each camera
        self.cam1 = CANTransmitter(channel="can0", arbitration_id=0x080)
        self.cam2 = CANTransmitter(channel="can0", arbitration_id=0x081)

        # Create the service
        self.service = self.create_service(GimbalCommand, "/electronics/gimbal_command", self.gimbal_callback)


    # Method that sends data over the CAN lines to the gimbal cameras
    def gimbal_callback (self, request: GimbalCommand.Request, response: GimbalCommand.Response):

        # Check for each camera
        can = self.cam1 if request.id == 1 else self.cam2

        # Convert the position to bytes
        byte_data = int.to_bytes(request.position, 1, "big")

        # Transmit the bytes
        can.transmit(byte_data)

        # Return a success
        response.success = True
        return response
    

# Main function sets up the ROS class
def main(args=None):

    # Create the publisher
    rclpy.init(args = args)
    publisher = GimbalService()
    rclpy.spin(publisher)

    # Clean up when complete
    publisher.destroy_node()
    rclpy.shutdown()


# Called when the script executes
if __name__=="__main__":
    main()
