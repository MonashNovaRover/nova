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
EDITED:		26/02/2022
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

        # Set up the CAN interface for the CAN 1 lines
        try:
            self.arm_gimbal = CANTransmitter(channel="can1", arbitration_id=0x080)
            self.beacon_gimbal = self.arm_gimbal
        except:
            print("CAN 1 Network not found!")
            exit()
        
        # Set up the CAN interface for the CAN 2 lines
        try:
            self.mast_gimbal = CANTransmitter(channel="can0", arbitration_id=0x080)            
        except:
            print("CAN 0 Network not found!")
            exit()

        # Create the service
        self.service = self.create_service(GimbalCommand, "/electronics/gimbal_command", self.gimbal_callback)


    # Method that sends data over the CAN lines to the gimbal cameras
    def gimbal_callback (self, request: GimbalCommand.Request, response: GimbalCommand.Response):

        # Get the gimbal based on the id
        gimbal = self.beacon_gimbal                         # 3
        if request.id == 1: gimbal = self.arm_gimbal        # 2
        elif request.id == 2: gimbal = self.mast_gimbal     # 1

        # Convert the angles to bytes
        byte_x = int.to_bytes(request.angle_x, 1, "big")
        byte_y = int.to_bytes(request.angle_y, 1, "big")

        # Transmit the bytes
        gimbal.transmit(byte_x + byte_y)

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
