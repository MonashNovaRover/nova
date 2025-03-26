#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This file contains the RFID service to receive service requests, which are then 
transmitted to the controlling arduino as appropriate, with the text response being
returned to the calling client for use as required
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:     electronics
AUTHOR(S):    Josh Cherubino, Bailey Chessum
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
from nova_interfaces.srv import RFIDCommand

import argparse
import rclpy, time
from rclpy.node import Node
from urllib.parse import quote
from serial import Serial

from std_msgs.msg import String
from std_srvs.srv import Trigger

class RFIDService(Node):

    def __init__(self):
        super().__init__('rfid_service')
        
        self.declare_parameter('port', '/dev/ttyUSB1')
        self.get_logger().info(f"Using port {self.get_parameter('port').value}.")
        self.get_logger().info("Waiting for RFID scanner to be plugged in.")

        # Loop until the RFID is plugged in
        while True:
            try:
                self.ser = Serial(baudrate = 115200, port = self.get_parameter('port').value) # TODO: Check port
                break
            except:
                time.sleep(0.1)
            
        self.srv = self.create_service(RFIDCommand, '/electronics/rfid_service', self.handle_rfid_request)

        self.read_service = self.create_service(Trigger, '/electronics/rfid/read', self.handle_read)

        self.unprocessed = ""

        self.service_arduino_timer = self.create_timer(0.05, self.service_arduino)

        self.data_publisher = self.create_publisher(String, "/electronics/rfid/data", 10)

        self.EOM = b'\n' # carriage ret for EOM
        self.get_logger().set_level(10) # FOR DEBUGGING
        self.get_logger().info('Started RFID service successfully!')



    def handle_rfid_request(self, request: RFIDCommand, response):
        self.get_logger().info(f'Processing {request.command} RFID request')
        self.get_logger().debug(f'Received request: {request.command}\n{request.data}')
        cmd = request.command.lower()

        if cmd in ['read', 'clear', 'restart', 'dump']:
            self.write_msg(cmd)
            response.response = "output has been moved" #self.read_data()
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



    def handle_read(self, request: Trigger, response):
        self.write_msg("read")

        response.success = False
        response.message = "No response"

        data = self.get_non_debug_command_from_arduino()
        while data is None:
            data = self.get_non_debug_command_from_arduino()

        [command, args] = data

        if command != "data":
            response.message = f"{command} {args}"
            response.success = False

            self.process_arduino_command(command, args)
            return response

        response.success = True
        self.on_received_data(args)

        # Try to get a response message, if any
        secondary_data = self.get_non_debug_command_from_arduino()
        if secondary_data is not None:
            [secondary_command, secondary_args] = data
            self.process_arduino_command(secondary_command, secondary_args)
            response.message = f"{secondary_command} {secondary_args}"
        else:
            response.message = ""

        return response
    
    def read_data(self) -> str:
        '''
        Read transmitted data terminated with EOM
        '''
        # read until EOM
        self.get_logger().debug('Reading data')

        # input_buffer_length = self.ser.in_waiting
        # data = self.ser.read(size=input_buffer_length)
        #data = data.strip(self.EOM) # remove EOM from response
        # data = data.strip(b'\0') # strip any null chars from data
        # self.get_logger().debug('data')

        # print(data) #print raw bytes

        # return as string
        try:
            return "e"# str(data)
        except Exception:
            self.get_logger().error('Failed to decode RFID arduino response')
            # dump raw hex to logger
            #self.get_logger().error(f'Raw data: {data}')
            return 'Service error: Failed to decode message'


    def get_command_from_arduino(self):
        if self.ser.in_waiting > 0:
            rawdata = self.ser.read(self.ser.in_waiting)
            try:
                new_text = rawdata.decode("ascii")
                self.unprocessed += new_text
            except:
                self.get_logger().error(f"Failed to decode raw data into ascii: \"{quote(rawdata)}\"")


        split_unprocessed_data = self.unprocessed.split('\n', 1)
        if len(split_unprocessed_data) != 2:
            return None

        line = split_unprocessed_data[0]
        self.unprocessed = split_unprocessed_data[1]

        if len(line.strip()) == 0:
            return None

        command = ""
        args = ""

        split_data = line.split(' ', 1)
        if len(split_data) == 2:
            [command, args] = split_data
        elif len(split_data) == 1:
            command = line

        command = command.lower().strip(':').strip()

        return [command, args]

    def get_non_debug_command_from_arduino(self):
        data = self.get_command_from_arduino()

        while data is not None and data[0] == "debug":
            self.process_arduino_command(data[0], data[1])
            data = self.get_command_from_arduino()

        return data

    def service_arduino(self):
        """
        Handles any data from the arduino
        """
        data = self.get_command_from_arduino()
        while data is not None:
            [command, args] = data
            self.process_arduino_command(command, args)

            data = self.get_command_from_arduino()

    def process_arduino_command(self, command: str, args: str):
        """
        Using data sent over serial by the arduino, call an appropriate function to handle the message.
        :return: None
        """

        if command == "data":
            self.on_received_data(args)
        elif command == "error":
            self.get_logger().error("Arduino: " + args)
        elif command == "info":
            self.get_logger().info("Arduino: " + args)
        elif command == "debug":
            self.get_logger().debug("Arduino: " + args)
        elif command == "warn" or command == "warning":
            self.get_logger().warn("Arduino: " + args)
        else:
            self.get_logger().warn(f"Arduino sent unknown command \"{quote(command)}\" with args \"{quote(args)}\"")

    def on_received_data(self, data):
        data_values = data.strip().split(' ')

        # Try convert
        def process_hex(value: str) -> str:
            try:
                return chr(int(value, 16))
            except:
                self.get_logger().warn(f"Tried to parse \"{quote(value)}\" as hex value when reading data.")
                return '\0'

        chars = [process_hex(x) for x in data_values]
        txt = ''.join(chars)

        txt = txt.strip()
        txt = txt.strip('\0').rstrip()

        self.get_logger().info(f"data: \"{txt}\".")

        msg = String()
        msg.data = txt

        self.data_publisher.publish(msg)

        return msg

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

def cli_parser():
    parser = argparse.ArgumentParser(description="The RFID node", usage="rfid [-p PORT]")
    parser.add_argument("-p", "--port", type=str, default="/dev/ttyELBAUMRFID69420", help="the port to use for usb serial")
    args = parser.parse_args()

    return args

def main(args=None):
    rclpy.init(args=args)

    # args2 = cli_parser()

    rfid_service = RFIDService()

    rclpy.spin(rfid_service)

    rfid_service.destroy_node()

    rclpy.shutdown()


if __name__ == '__main__':
    main()
