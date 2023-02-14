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

import rclpy, time
from rclpy.node import Node

from serial import Serial

class RFIDService(Node):

    def __init__(self):
        super().__init__('rfid_service')

        self.get_logger().info("Waiting for RFID scanner to be plugged in.")

        # Loop until the RFID is plugged in
        while True:
            try:
                self.ser = Serial(baudrate = 115200, port = '/dev/ttyELBAUMRFID69420') # TODO: Check port
                break
            except:
                time.sleep(1.0)
            
        self.srv = self.create_service(RFIDCommand, '/electronics/rfid_service', self.handle_rfid_request)   
        self.EOM = b'\r' # carriage ret for EOM
        self.get_logger().set_level(10) # FOR DEBUGGING
        self.get_logger().info('Started RFID service successfully!')

    def handle_rfid_request(self, request: RFIDCommand, response):
        self.get_logger().info(f'Processing {request.command} RFID request')
        self.get_logger().debug(f'Received request: {request.command}\n{request.data}')
        cmd = request.command.lower()

        if cmd in ['read', 'clear', 'restart']:
            self.write_msg(cmd)
            response.response = self.read_data()
        elif cmd in ['write', 'poll']:
            self.write_msg(cmd)
            self.write_msg(request.data)
            response.response = self.read_data()
        else:
            # catch invalid commands
            msg = f'Service request refused: Invalid command: {cmd}'
            self.get_logger().error(msg)
            response.response = msg
            return response
        
        try:
            self.get_logger().debug(f'Response received from arduino: {response.response}')
        except Exception:
            # sometimes crashes here which we don't want to do. If we can't log its not a massive issue
            pass
        return response
    
    def read_data(self) -> str:
        '''
        Read transmitted data terminated with EOM
        '''
        # read until EOM
        self.get_logger().debug('Reading data')
        data = self.ser.read_until(self.EOM)
        data = data.rstrip(self.EOM) # remove EOM from response
        data = data.rstrip(b'\0') # strip any null chars from data
        self.get_logger().debug('data')
        print(data) #print raw bytes
        # return as string
        try:
            decoded = data.decode('ascii')
            return decoded
        except Exception:
            self.get_logger().error('Failed to decode RFID arduino response')
            # dump raw hex to logger
            self.get_logger().error(f'Raw data: {data}')
            return 'Service error: Failed to decode message'

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
        data.append(ord(self.EOM)) #add EOM

        CHUNK_SIZE = 60

        for i in range(0, len(data), CHUNK_SIZE):
            self.ser.write(data[i:i+CHUNK_SIZE])
            self.get_logger().debug(f'Chunk sent')
            time.sleep(0.1)

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
