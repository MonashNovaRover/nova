#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This file contains the ROS2 receiver code for the hydraprobe publisher
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ PACKAGE:     electronics 
AUTHOR(S):    Josh Cherubino
CREATION:    12/02/2022
EDITED:      12/02/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
    - Add QoS profile
    - Test and bugfix
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""  

from typing import Union, List
from coms_utils.uart_interface import UARTTransceiver

import rclpy
import time
from rclpy.node import Node

from core.msg import HydraprobeData 

class HydraprobeTransceiver(UARTTransceiver): 
    '''
    Class to handle configuring and communicating with moisture probe via USB to RS485 interface
    See datasheet for communication specification:
        https://www.stevenswater.com/resources/documentation/hydraprobe/HydraProbe_Manual_Jan_2018.pdf
    '''

    def __init__(self, probe_address: str = "000", **kwargs):
        super().__init__(**kwargs)

        if len(probe_address) != 3:
            raise ValueError('Probe address must be 3 bytes long')

        self._probe_address = probe_address
        
    def transmit(self, data: str) -> bool:
        '''
        Custom transmit function to handle transmitting data.
        Communication must be terminated with CRLF and begin with 3 digits of
        address
        '''
        message = self._probe_address 
        message += data
        message += '\r\n' # add terminating CLRF
        packet = message.encode('ascii') # ascii encode data for transmission

        # Attempt to show information
        try:
            with self._lock:
                self.ser.write(packet)
            self.debug(f"Successfully transmitted data\n{data}")
            return True
        
        #if any errors occur then return failed status
        except self.serial.SerialTimeoutException:
            self.error(f"Transmit timeout occurred on {self.ser.name}")

        return False

    def receive(self) -> Union[bytes, None]:
        '''
        Custom receive function to read data until CRLF (\r\n) is 
        received. See https://stackoverflow.com/questions/16470903/pyserial-2-6-specify-end-of-line-in-readline
        '''
        eol = b'\r\n'
        leneol = len(eol)
        line = bytearray()
        while True:
            with self._lock:
                c = self.ser.read(1)
            if  len(c) < 1:
                self.error(f"Read timeout on {self.ser.name} bytes")
                return None 
            line += c
            if line[-leneol:] == eol:
                break

        # strip address
        return line[len(self._probe_address):]
    
    def handle(self, data: bytes) -> List[float]:
        '''
        Custom handle function to decode data into string.
        Reading set values are divided by comma so we can split data accordingly
        ''' 
        decoded = data.decode('ascii') 
        return [float(val) for val in decoded.split(',')] 
        
    def update_readings(self) -> bool:
        '''
        Function to update internal readings in sensor
        Returns success status
        '''
        self.debug(f'Updating internal sensor readings')
        return self.transmit('TR')

    def get_reading_set(self, set_number: int = 2) -> Union[List[float], None]:
        '''
        Reads a particular reading set from the sensor
        '''
        self.debug(f'Getting reading set {set_number}')
        if not self.transmit(f'T{set_number}'):
            # request failed
            return None

        # read response
        ret = self.receive()
        if ret is None:
            # read failed
            return None

        # decode returned values
        return self.handle(ret)


class HydraprobePublisher(Node):

    # Stores the port of the hydraprobe
    port: str = '/dev/ttyUSB0'

    # Main constructor
    def __init__(self):

        super().__init__('hydraprobe_publisher')

        # TODO: Update to use actual QoS profile
        self.publisher_ = self.create_publisher(HydraprobeData, '/science/hydraprobe_data', 10)
        
        # Attempt to create the transceiver
        try:
            self.hydraprobe_transceiver = HydraprobeTransceiver(
                logger = self.get_logger(),
                baudrate = 9600, # confirm this
                port = self.port, # TODO: check this
                probe_address = '000', # TODO: check this. /// is broadcast address for the probes so should be fine to use as long as we only have 1 connected.
                )
        
        # Print error if missing device
        except:
            self.get_logger().error("\033[1;91m\nERROR: Unable to find device on '%s'.\033[0m" % self.port)
            exit()
        
        # Create the timer
        self.publisher_timer = self.create_timer(3, self.publish_values)

        # get firmware version
        self.hydraprobe_transceiver.transmit("FV=?")
        self.get_logger().debug(self.hydraprobe_transceiver.receive().decode('ascii'))


    def publish_values(self):
        # request reading set and wait till its ready
        self.hydraprobe_transceiver.update_readings()
        # pretty jank but we will roll with it
        time.sleep(2)
        #then read values
        msg = HydraprobeData()
        values = self.hydraprobe_transceiver.get_reading_set(set_number=2)
        if values is not None:
            # write values into msg
            # see linked datasheet for reference on data ordering
            msg.temperature = values[0]
            msg.moisture = values[2]
            msg.conductivity = values[4]
        else:
            # set error state with -1 for all values
            msg.temperature = msg.moisture = msg.conductivity = -1

        self.publisher_.publish(msg)

    def destroy_node(self):
        '''
        Override for default node destruction
        '''
        self.hydraprobe_transceiver.close()
        return super().destroy_node()

def main(args=None):
    rclpy.init(args=args)

    pub = HydraprobePublisher()

    rclpy.spin(pub)

    # Destroy the node explicitly
    pub.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
