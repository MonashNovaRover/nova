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
    # reading_sets = {0: [{"base_reg": 0x0200, "num_regs": 6}], # get comms details
    #                 1: [{"base_reg": 0x0005, "num_regs": 1},    # get EC and dielectric constant
    #                     {"base_reg": 0x0002, "num_regs": 1}],
    #                 2: [{"base_reg": 0x0000, "num_regs": 3}],
    #                 3: [{"base_reg": 0x0000, "num_regs": 3},
    #                     {"base_reg": 0x0005, "num_regs": 1}]}   # get temp, moisture, EC

    def __init__(self, port, logger, baudrate=9600, bytesize=8, parity='N', stopbits=1, retries=1, broadcast_enable=True):
        self.logger = logger
        self.client = ModbusSerialClient(port, baudrate=baudrate, bytesize=bytesize, parity=parity, stopbits=stopbits, retries=retries, broadcast_enable=broadcast_enable)
        if not self.client.connect():
            raise RuntimeError("Failed to run self.client.connect()")

    def read_moisture(self, slave=1):
        client = self.client
        regs = client.read_holding_registers(0x0001, count=1, slave=1)
        try:
            val = regs.registers[0]/100
        except AttributeError:
            val = self.read_moisture(slave)

        return val

    def read_temp(self, slave=1):
        client = self.client
        regs = client.read_holding_registers(0x0000, count=1, slave=1)
        try:
            val = regs.registers[0]/100
        except AttributeError:
            val = self.read_temp(slave)

        return val

    def read_ec(self, slave=1):
        client = self.client
        regs = client.read_holding_registers(0x0002, count=1, slave=1)
        try:
            val = regs.registers[0]
        except AttributeError:
            val = self.read_ec(slave)

        return val

    def read_epsilon(self, slave=1):
        client = self.client
        regs = client.read_holding_registers(0x0005, count=1, slave=1)
        try:
            val = regs.registers[0]/100
        except AttributeError:
            val = self.read_temp(slave)

        return val

    def read_all(self, slave=1):
        self.logger.info("Request recieved: Starting to read values")
        client = self.client
        ec = self.read_ec(slave)
        self.logger.info(f"EC value: {ec}")
        time.sleep(0.1)
        moisture = self.read_moisture(slave)
        self.logger.info(f"Moisture value: {moisture}")
        time.sleep(0.1)
        temp = self.read_temp(slave)
        self.logger.info(f"Temp value: {temp}")
        time.sleep(0.1)
        eps = self.read_epsilon(slave)
        self.logger.info(f"Dielectric value: {eps}")

        return [temp, moisture, ec, eps]

    def set_soil_type(self, soil, slave=1):
        client = self.client
        match soil.lower():
            case 'mineral':
                soil = 0
            case 'sand':
                soil = 1
            case 'clay':
                soil = 2
            case 'organic':
                soil = 3
            case _:
                raise ValueError(f"Soil type {soil} is invalid: please select one of {['sand', 'mineral', 'clay', 'organic']}")

        client.write_register(0x0020, soil, count=1, slave=slave)

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
        self.__port = self.declare_parameter("port", "/dev/serial/by-id/usb-FTDI_FT232R_USB_UART_AQ018NTV-if00-port0")
        self.__soil = self.declare_parameter("soil", "sand")
        self.__filepath = self.declare_parameter("filepath", "")
        self.data = []
        # To change the port value, enter "ros2 run science hydraprobe_publisher.py --ros-args -p port:='<port_value>'"
        # To change the soil value, enter "ros2 run science hydraprobe_publisher.py --ros-args -p soil:='<soil_value>'"
        # Note you can stack multiple param values, e.g. "--ros-args -p port:='<port_value>' soil:='<soil_value>' ..."
        # Attempt to create the transceiver
        try:
            self.hydraprobe_transceiver = NewHydraprobeTransceiver(
                port = self.__port.value, 
                baudrate = 9600, # confirm this
                logger = self.get_logger(),
                )
        
        # Print error if missing device
        except Exception as e:
            self.get_logger().error("\033[1;91m\nERROR: Unable to find device on '%s'.\033[0m" % self.__port.value)
            raise e 
        
        # Set soil type
        self.hydraprobe_transceiver.set_soil_type(self.__soil.value) 

        # Create the timer
        self.publisher_timer = self.create_timer(0.5, self.publish_values)
        self.get_logger().info("Hydraprobe started")

        # get firmware version xx no longer applicable
        # self.hydraprobe_transceiver.transmit("FV=?")
        # self.get_logger().debug(self.hydraprobe_transceiver.receive().decode('ascii'))

    def read_all_test(self):
        """
        This is purely to test/bugfix the code without access to 
        the physical hydraprobe (requires commenting out the "raise 
        e" when connecting to the probe).
        """
        time.sleep(0.1)
        time.sleep(0.1)
        time.sleep(0.1)

        return [1, 2, 3, 4]

    def publish_values(self):
        """
        Note that to save values, you must enter this command in
        another terminal:
        "ros2 topic echo /science/hydraprobe_data > <filepath/filename>"
        For example, the command:
        "ros2 topic echo /science/hydraprobe_data > ~/Downloads/data.yaml"
        Will save the data as a .yaml file (note it will still do 
        this if you omit the .yaml) at that location
        """
        # request reading set and wait till its ready  xx deprecated
        # self.hydraprobe_transceiver.update_readings()     
        # # pretty jank but we will roll with it
        # time.sleep(2)
        #then read values
        msg = HydraprobeData()
        values = self.hydraprobe_transceiver.read_all()
        # values = self.read_all_test()
        if values is not None:
            self.data.append(values)
            # write values into msg
            # see linked datasheet for reference on data ordering
            msg.temperature = float(values[0])
            msg.moisture = float(values[1])
            msg.conductivity = float(values[2])
            msg.dielectric = float(values[3])
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
        self.save_data()
        return super().destroy_node()

def main(args=None):
    rclpy.init(args=args)
    node = HydraprobePublisher()
    rclpy.spin(node)
    # Destroy the node explicitly
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
