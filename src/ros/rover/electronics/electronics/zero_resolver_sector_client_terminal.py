#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This file contains a command-line interface to call the zero_resolver_sector service
and print out the response
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: zero_resolver_sector_client_terminal
TOPICS: None
SERVICES:
  - /electronics/resolver_sector_zero_service    [core/StringTrigger]        [Client]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:     electronics
AUTHOR(S):   Jory Braun
CREATION:    08/05/2023
EDITED:      08/05/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
from core.srv import StringTrigger
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


class ZeroResolverSectorClient(Node):

    def __init__(self):
        super().__init__('zero_resolver_sector_client_terminal')
        self.cli = self.create_client(StringTrigger, '/electronics/resolver_sector_zero_service')
        while not self.cli.wait_for_service(timeout_sec=1.0):
            print('Service not available')
        self.future = None

    def send_request(self, args):
        req = StringTrigger.Request()
        req.value = args.joint

        self.future = self.cli.call_async(req)

def cli_parser():
    # Include only geared resolvers 
    resolver_choices = [
        'j6',
    ]
    resolver_choices_string = "{" + ",".join(resolver_choices) + "}"
    parser = DefaultHelpParser(description="Reset the current sector to 0 for a geared resolver", usage=f"zero_resolver_sector [-h] {resolver_choices_string}")
    parser.add_argument("joint", type=str, choices=resolver_choices, help="the joint to zero")
    args = parser.parse_args()

    return args


def main():
    rclpy.init()

    args = cli_parser()

    zero_resolver_sector_client = ZeroResolverSectorClient()
    zero_resolver_sector_client.send_request(args)

    rclpy.spin_until_future_complete(zero_resolver_sector_client, zero_resolver_sector_client.future)
    
    try:
        # get and print text data to user
        response = zero_resolver_sector_client.future.result()
        print("Success" if response.success else "Fail")
        print(response.message)
    except Exception as e:
        print('[Error]: Service call failed')

    zero_resolver_sector_client.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
