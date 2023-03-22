#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This node interfaces with the science CAN terminals
and is able to send data across the CAN network.
It converts a science command, from the GUI, into a
CAN command to the science payload.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: science_transmitter
SERVICES:
  - /science/science_transmitter   [ScienceCommand]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	science
AUTHOR(S):	Miles Higgins, Harrison Verrios, Niko Verrios
CREATION:	15/02/2022
EDITED:		21/03/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

# Include all ROS dependencies
import rclpy
from rclpy.node import Node
from rclpy.qos import QoSReliabilityPolicy, QoSProfile
from rclpy.subscription import SubscriptionEventCallbacks
from rclpy.duration import Duration

from core.srv import ScienceCommand
from core.msg import Heartbeat
# Include other dependencies
import json, os
import jcan



'''
Science node class that acts as a ttransmitter for the science data
over the CAN lines. It interfaces with a ROS service that can communicate
the commands.
'''
class ServiceNode(Node):

    def __init__(self):

        # Initialise the node
        super().__init__('science_transmitter')

        # Print initialisation information
        self.get_logger().warning("\033[92;1mInitialising the Science Transmitter class.\033[0m")

        # Get the directory of the JSON file
        directory = os.path.expanduser('~') + "/nova_ws/src/rover/science/configs/commands.json"

        # Reads the command json data
        with open (directory, "r") as file:
            self.archive: dict  = json.loads(file.read())
        
        # Store the list of targets
        self.targets = list(self.archive["targets"].keys())

        # Store the list of argument decoding
        self.arg_encoding = self.archive["arg_encoding"]

        # Initialise all the data frames as found in commands.json
        self.can_frame_data = self.can_frame_initialisation()

        #declare parameters
        self.declare_parameter("canbus", "can1")

        deadline = Duration(nanoseconds=2e8)
        events = SubscriptionEventCallbacks(deadline=self.deadline_callback)
        self.qos = QoSProfile(reliability=QoSReliabilityPolicy.BEST_EFFORT, depth=1, deadline=deadline)
        #heartbeat
        self.heartbeat_sub = self.create_subscription(Heartbeat, '/science/heartbeat', self.heartbeat_callback,
                                                      self.qos, event_callbacks=events)

        # Initialise the jcan bus
        try:
            self.bus = jcan.Bus()
            # Open the can bus
            self.bus.open(self.get_parameter("canbus").value)
        except:
            self.get_logger().error("\033[91;1m\nTransmitter ERROR: Unable to find can1 network.\033[0m")
            exit()


        # Start the service with the callback
        self.service = self.create_service(ScienceCommand, '/science/transmitter', self.callback_func)

        self.can_timer = self.create_timer(0.5, self.can_send_callback)



    def execute_can_msg(self, arb_id, action_id, arg_id):
        command = action_id + arg_id
        try:
            command_hex_list = list(bytearray.fromhex(command))
            arb_hex = int("0x" + arb_id, 16)

            self.bus.send(jcan.Frame(arb_hex, command_hex_list))
        except Exception as e:
            self.get_logger().error("\033[[1;91m\nTransmitter FAILED! Exception: %s; Command: %s#%s\033[0m" % (e, arb_id, command))
            return False
        else:
            self.get_logger().warning("\033[1;92m\nTransmitter SUCCESS! Command: %s#%s\033[0m" % (arb_id, command))
            return True


    def deadline_callback(self, deadline_info):
        self.get_logger().warning("\033[1;92m\nTransmitter 200ms Deadline exceeded!")
        self.can_frame_data = self.can_frame_initialisation()


    def can_send_callback(self):
        for arb_id in self.can_frame_data.keys():
            for action_id in self.can_frame_data[arb_id].keys():
                arg_id = self.can_frame_data[arb_id][action_id]
                if arg_id is not None:
                    success = self.execute_can_msg(arb_id, action_id, arg_id)
                    if success and (int(arg_id, 16) == 0 or (action_id != "" and int(action_id, 16) == 13)):
                        self.can_frame_data[arb_id][action_id] = None
        

    def can_frame_initialisation(self):
        # Initialising the CAN frame data based on the commands.json file.
        frame_data = dict()
        for target in self.targets:
            target_hex = self.archive['targets'][target]['hex']
            frame_data[target_hex] = dict()
            for action in self.archive['targets'][target]['actions']:
                action_hex = self.archive['targets'][target]['actions'][action]['hex']
                frame_data[target_hex][action_hex] = None

        return frame_data


    def heartbeat_callback(self, msg):
        pass


    def get_args(self, action, args):

        arg_id = ""
        # Go through each of the actions and ensure they exist
        for arg in action["args"]:
            # Ensure it exists
            if arg not in args.keys():
                raise Exception("Missing argument '%s'." % arg)

            # If it does exist, check the decoder
            value = args[arg]

            # Check for non-number argument
            if str(value).lower() not in self.arg_encoding.keys():
                arg_id += format(int(value), "x").rjust(2, "0")

            # Get the decoded value and add to the list
            else:
                arg_id += self.arg_encoding[str(value).lower()]
        return arg_id
        
    '''
    The callback function is execited when a command is received by the transmitter.
    It takes in a request and a response message and outputs the associated
    response from the service.

    '''
    def callback_func(self, request, response):

        # Load the data
        try:
            # Reads the command
            command_data = json.loads(request.command)
            target = command_data["target"].lower()
            action = command_data["action"].lower()
            args = command_data["args"]
            
            # Check for an invalid target entered in the command
            if target not in self.targets:
                raise Exception("Invalid target '%s'." % target)

            # Grab the arbitration ID
            arb_id = self.archive["targets"][target]["hex"]

            # Set the target data
            target = self.archive["targets"][target]

            # Check if the action exists
            if action not in target["actions"].keys():
                raise Exception("Invalid action '%s'." % action)

            # Grab the action ID
            action_id = target["actions"][action]["hex"]

            # Set the action target
            action = target["actions"][action]

            # Store an argument id
            arg_id = self.get_args(action, args)

            self.can_frame_data[arb_id][action_id] = arg_id
        
        # If an error occurred
        except Exception as e:
            self.get_logger().error("\033[1;91m\nTransmitter ERROR! Failed to parse science command because:\n\t%s\033[0m" % e)
            
            # Process failed
            response.success = False

        # Return the response data
        return response



# When the script starts, it runs the science class and waits until completion
def main (args = None):
    rclpy.init(args = args)
    service = ServiceNode()
    rclpy.spin(service)
    rclpy.shutdown()


# Called when the script starts
if __name__ == '__main__':
    main()
