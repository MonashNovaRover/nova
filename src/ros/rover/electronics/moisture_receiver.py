#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This file contains the ROS2 receiver code for the moisture probe
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:     electronics 
AUTHOR(S):    Josh Cherubino
CREATION:    12/02/2022
EDITED:      12/02/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
    -
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""  

from typing import Union
from coms_utils.uart_interface import UARTTransceiver

class MoistureProbeTransceiver(UARTTransceiver):
    '''
    Class to handle configuring and communicating with moisture probe via USB to RS485 interface
    See datasheet for communication specification:
        https://www.stevenswater.com/resources/documentation/hydraprobe/HydraProbe_Manual_Jan_2018.pdf
    '''

    def __init__(self, probe_address: str = "000", **kwargs):
        super().__init(**kwargs)

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

        try:
            with self._lock:
                self.ser.write(packet)
            self.debug(f"Successfully transmitted data\n{data}")
            return True
        #if any errors occur then return failed status
        except serial.SerialTimeoutException:
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

        return line
    
    def handle(self, data: bytes) -> str:
        '''
        Custom handle function to decode data into string
        '''
        return data.decode('utf-8')


