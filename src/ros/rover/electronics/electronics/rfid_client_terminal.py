#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This file contains a command-line interface to call the rfid service and
print out the response
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: None
TOPICS: None
SERVICES:
  - /electronics/rfid_service    [core/RFIDCommand]        [Client]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:     electronics
AUTHOR(S):   Jory Braun, Josh Cherubino
CREATION:    09/03/2022
EDITED:      29/04/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
from core.srv import RFIDCommand
import rclpy
from rclpy.node import Node

import argparse
import sys


class DefaultHelpParser(argparse.ArgumentParser):
    def error(self, message):
        """
        Override the error function to also print the help
        """
        sys.stderr.write(f"error: {message}\n")
        self.print_help()
        sys.exit(2)


class RFIDClient(Node):

    def __init__(self):
        super().__init__('rfid_client_terminal')
        self.cli = self.create_client(RFIDCommand, '/electronics/rfid_service')
        while not self.cli.wait_for_service(timeout_sec=1.0):
            print('Service not available')
        self.future = None

    def send_request(self, args):
        req = RFIDCommand.Request()
        req.command = args.command
        if args.data is not None:
            req.data = args.data

        self.future = self.cli.call_async(req)

def cli_parser():
    parser = DefaultHelpParser(description="Send or receive using the RFID scanner", usage="rfid [-h] {read,clear,restart,dump,write,poll} [-d DATA]")
    parser.add_argument("command", type=str, choices=['read', 'clear', 'restart', 'dump', 'write', 'poll'], default='read', help="Command to send to the RFID reader")
    parser.add_argument("-d", "--data", type=str, default=None, help="Data to write to the RFID reader. Only required if using the 'write' or 'poll' commands")
    args = parser.parse_args()
    
    # Require data for some instructions
    if args.data is None and args.command in ['write', 'poll']:
        parser.error(f"command '{args.command}' requires data")
    
    # Only use data if writing, otherwise delete
    if args.data is not None and args.command not in ['write', 'poll']:
        print("[Warning]: Data given for a command that does not need data. Ignoring data")
        args.data = None

    return args


def main():
    rclpy.init()

    args = cli_parser()

    rfid_client = RFIDClient()
    rfid_client.send_request(args)

    rclpy.spin_until_future_complete(rfid_client, rfid_client.future)
    
    try:
        # get and print text data to user
        response = rfid_client.future.result()
        print(response.response)
    except Exception as e:
        print('[Error]: Service call failed')

    rfid_client.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
