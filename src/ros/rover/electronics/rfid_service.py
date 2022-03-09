#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This file contains the RFID service to receive service requests, which are then 
transmitted to the controlling arduino as appropriate, with the text response being
returned to the calling client for use as required
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:     electronics
AUTHOR(S):    Josh Cherubino
CREATION:    not sure lol
EDITED:      9/3/2022 by Josh Cherubino
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Available services:
    - /electronics/rfid_service with core/srv/RFIDCommand type
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
    - Add timeouts to reading
    - Add more sophisticated write error handling
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
from core.srv import RFIDCommand

import rclpy
from rclpy.node import Node

from serial import Serial

class RFIDService(Node):

    def __init__(self):
        super().__init__('rfid_service')
        self.srv = self.create_service(RFIDCommand, '/electronics/rfid_service', self.handle_rfid_request)
        self.ser = Serial(baudrate = 115200, port = '/dev/ttyUSB0') # TODO: Check port
        self.EOM = b'\r' # carriage ret for EOM
        #self.get_logger().set_level(10) # FOR DEBUGGING
        self.get_logger().info('Started RFID service')

    def handle_rfid_request(self, request: RFIDCommand, response):
        self.get_logger().info(f'Processing {request.command} RFID request')
        self.get_logger().debug(f'Received request: {request.command}\n{request.data}')
        cmd = request.command.lower()

        if cmd in ['read', 'clear', 'restart']:
            self.write_msg(cmd)
            response.response = self.read_data()
        elif cmd == 'write':
            self.write_msg(cmd)
            self.write_msg(request.data)
            response.response = self.read_data()
        else:
            # catch invalid commands
            msg = f'Service request refused: Invalid command: {cmd}'
            self.get_logger().error(msg)
            response.response = msg
            return response
        
        self.get_logger().debug(f'Response received from arduino: {response.response}')
        return response
    
    def read_data(self) -> str:
        '''
        Read transmitted data terminated with EOM
        '''
        # read until EOM
        self.get_logger().debug('Reading data')
        data = self.ser.read_until(expected=self.EOM)
        data = data.rstrip(self.EOM) # remove EOM from response
        data = data.rstrip(b'\0') # strip any null chars from data
        # return as string
        return data.decode('ascii')

    def write_msg(self, msg: str):
        '''
        Write ascii encoded bytearray data to arduino.
        '''
        self.get_logger().debug(f'Message to write: {msg}')
        # N.B. Somehow ROS was causing null char issues when trying to read
        # what had just been written to card. Removing with rstrip seemed to 
        # solve although this doesn't make much sense
        msg.rstrip('\0') 
        data = bytearray(msg, encoding='ascii')
        data.append(ord(self.EOM)) # indicate end of message
        self.ser.write(data)

    def destroy_node(self):
        self.get_logger().info('Closing serial connection')
        self.ser.close()
        return super().destroy_node()

def main(args=None):
    rclpy.init(args=args)

    rfid_service = RFIDService()

    rclpy.spin(rfid_service)

    rfid_service.destroy_node()

    rclpy.shutdown()


if __name__ == '__main__':
    main()
