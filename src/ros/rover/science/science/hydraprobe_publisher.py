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
from pymodbus.client import ModbusSerialClient

import rclpy
import time
from rclpy.node import Node

from nova_interfaces.msg import HydraprobeData 
import logging
import logging.handlers as Handlers

pymodbuslog = logging.getLogger('pymodbus')
pymodbuslog.setLevel(logging.ERROR)


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

class NewHydraprobeTransceiver():
    reading_sets = {0: [{"base_reg": 0x0200, "num_regs": 6}], # get comms details
                    1: [{"base_reg": 0x0005, "num_regs": 1},    # get EC and dielectric constant
                        {"base_reg": 0x0002, "num_regs": 1}],
                    2: [{"base_reg": 0x0000, "num_regs": 3}],
                    3: [{"base_reg": 0x0000, "num_regs": 3},
                        {"base_reg": 0x0005, "num_regs": 1}]}   # get temp, moisture, EC


    def __init__(self, port, logger, probe_address=1, baudrate=9600, bytesize=8, parity='N', stopbits=1, retries=1, broadcast_enable=True):
        self.client = ModbusSerialClient(port, baudrate=baudrate, bytesize=bytesize, parity=parity, stopbits=stopbits, retries=retries, broadcast_enable=broadcast_enable)
        self.addr = probe_address
        self.logger = logger

        if not self.client.connect():
            raise RuntimeError("Failed to run self.client.connect()")

    def get_reading_set(self, set_number: int = 0):
        read_set = self.reading_sets[set_number]
        results = []

        for item in read_set:
            reply = self.read_registers(item["base_reg"], item["num_regs"])

            try:
                results += reply.registers
            except Exception as e:
                self.logger.error(f"The results varaible is {results}")
                self.logger.error(str(e))
                return None
        
        return results

    def write_registers(self, base_reg, values: list[int] | int):
        self.client.write_registers(base_reg, values, self.addr)

    def read_registers(self, base_reg, num_regs: int = 1):
        reply = self.client.read_holding_registers(base_reg, num_regs, self.addr)
        
        try:
            regs = reply.registers
        except AttributeError as e:
            self.logger.error(f"failed with regs {reply}: {str(e)}")
            regs = None

        return regs
    
    def close(self):
        self.client.close()


class HydraprobePublisher(Node):

    # Stores the port of the hydraprobe
    # port: str = '/dev/ttyUSB0'

    # Main constructor
    def __init__(self):

        super().__init__('hydraprobe_publisher')

        pymodbuslog.addHandler(Handlers.RotatingFileHandler("hydraprobe-logfile.txt", maxBytes=1024*1024))

        # TODO: Update to use actual QoS profile
        self.publisher_ = self.create_publisher(HydraprobeData, '/science/hydraprobe_data', 10)
        self.__port = self.declare_parameter("port", "/dev/ttyUSB0")
        
        # Attempt to create the transceiver
        try:
            # self.hydraprobe_transceiver = HydraprobeTransceiver(
            #     logger = self.get_logger(),
            #     baudrate = 9600, # confirm this
            #     port = self.port, # TODO: check this
            #     probe_address = 0, # TODO: check this. /// is broadcast address for the probes so should be fine to use as long as we only have 1 connected.
            #     )
            self.hydraprobe_transceiver = NewHydraprobeTransceiver(
                logger = self.get_logger(),
                baudrate = 9600, # confirm this
                port = self.__port.value, 
                probe_address = 0,
                )
        
        # Print error if missing device
        except Exception as e:
            self.get_logger().error("\033[1;91m\nERROR: Unable to find device on '%s'.\033[0m" % self.__port.value)
            raise e
        
        # Create the timer
        self.publisher_timer = self.create_timer(0.5, self.publish_values)
        self.get_logger().info("Hydraprobe started")

        # get firmware version xx no longer applicable
        # self.hydraprobe_transceiver.transmit("FV=?")
        # self.get_logger().debug(self.hydraprobe_transceiver.receive().decode('ascii'))


    def publish_values(self):
        # request reading set and wait till its ready  xx deprecated
        # self.hydraprobe_transceiver.update_readings()     
        # # pretty jank but we will roll with it
        # time.sleep(2)
        #then read values
        self.get_logger().info("Hydraprobe attempting to create message")

        msg = HydraprobeData()
        values = self.hydraprobe_transceiver.get_reading_set(set_number=3)
        if values is not None:
            # write values into msg
            # see linked datasheet for reference on data ordering
            msg.temperature = values[0]
            msg.moisture = values[1]
            msg.conductivity = values[2]
            msg.dielectric = values[3]
        else:
            # set error state with -1 for all values
            msg.temperature = msg.moisture = msg.conductivity = msg.dielectric = float(-1)

        self.get_logger().info(f"Publishing {msg.temperature}, {msg.moisture}, {msg.conductivity}, {msg.dielectric}")
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
