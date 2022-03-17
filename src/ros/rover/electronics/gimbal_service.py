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

# Constants numbers for the continuous rotation
CONTINUOUS_STATIONARY = 90
CONTINUOUS_LEFT = 45
CONTINIOUS_RIGHT = 135

# Main Gimbal Service class
class GimbalService (Node):

    # Store previous state for using the same gimbal for multiple data
    state = {
        "arm_x": 0,
        "arm_y": 0,
        "mast_x": 0,
        "mast_y": 0,
        "beacon": 0
    }


    # Constructor sets up the service
    def __init__ (self):
    
        # Set up the node
        super().__init__("gimbal_service")

        # Print initialisation information
        print("Initialising the Gimbal Service class.")

        # Set up the CAN interface for the CAN 1 lines
        try:
            self.can1_gimbal = CANTransmitter(channel="can1", arbitration_id=0x080)
        except:
            print("CAN 1 Network not found!")
            exit()
        
        # Set up the CAN interface for the CAN 0 lines
        try:
            self.can0_gimbal = CANTransmitter(channel="can0", arbitration_id=0x080)            
        except:
            print("CAN 0 Network not found!")
            exit()

        # Create the service
        self.service = self.create_service(GimbalCommand, "/electronics/gimbal_command", self.gimbal_callback)


    # Method that sends data over the CAN lines to the gimbal cameras
    def gimbal_callback (self, request: GimbalCommand.Request, response: GimbalCommand.Response):

        # Update the state data
        if request.id == 1:     # Arm Gimbal
            self.state["arm_x"] = request.angle_x
            self.state["arm_y"] = request.angle_y
        elif request.id == 2:   # Mast Gimbal
            self.state["mast_x"] = self.get_continuous_angle(request.angle_x)
            self.state["mast_y"] = request.angle_y
        else:                   # Beacon Gimbal
            self.state["beacon"] = request.angle_y + 0

        # Transmit the data for CAN 0
        if request.id == 1 or request.id == 3:
            byte_x = int.to_bytes(self.state["arm_x"], 1, "big")
            byte_y = int.to_bytes(self.state["arm_y"], 1, "big")
            byte_z = int.to_bytes(self.state["beacon"], 1, "big")
            self.can1_gimbal.transmit(byte_x + byte_y + byte_z)

        # Transmit the data for CAN 1
        else:
            byte_x = int.to_bytes(self.state["mast_x"], 1, "big")
            byte_y = int.to_bytes(self.state["mast_y"], 1, "big")
            self.can0_gimbal.transmit(byte_x + byte_y)

            # Reset the mast horizontal
            self.state["mast_x"] = self.get_continuous_angle(0)

        # Return a success
        response.success = True
        return response


    # Calculates the continuous angle to move to for velocity control    
    def get_continuous_angle (self, value):
        if value == 0:
            return CONTINUOUS_STATIONARY
        if value > 0:
            return CONTINIOUS_RIGHT
        if value < 0:
            return CONTINUOUS_LEFT
    

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
