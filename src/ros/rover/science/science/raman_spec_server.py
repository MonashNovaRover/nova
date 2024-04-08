#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: ROS Node for publishing responses to service requests from GUI data for CCD data for Raman Spectra
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: raman_spec_server
TOPICS: 
    - /science/raman_spec_msg [RamanSpectrum] [Publisher]
SERVICES: 
    - /science/raman_spec_srv [RamanSpec] [Server]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
SOURCE AUTHOR:	Esben Rossel <esbenrossel@gmail.com>
AUTHOR:         Connor Macdougall
CREATION:	    18/01/2024
EDITED:		    21/02/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Test in person
 - Determine whether wrong model of STM may be affecting outputs (Check CCD help of original code for more information)
 - Remove dummy pixels?

MORE INFO:
 - https://www.notion.so/Raman-Spectra-0161f5611e934a779247f3733ca8a608
"""

import logging
import rclpy
from rclpy.node import Node

from core.srv import RamanSpec
from core.msg import RamanSpectrum

import numpy as np
from serial import Serial, SerialException
import time
from typing import Tuple, List


class RamanServer(Node):
    # Constants set by firmware/hardware of STM32F103
    BAUDRATE = 115200
    MASTERCLOCK = 800000
    SPECTRA_SIZE = 3694
    OUTPUT_SIZE = 7388
    PHASE_SIGNAL = 3655

    def __init__(self):
        super().__init__('raman_spec_server')
        self.get_logger().set_level(logging.INFO)
        self.get_logger().info("Raman Spec Server starting")

        # initialising node values
        self.is_continuous = False
        self.continuous_settings = None, None, None, None   # A tuple of 4 values (port, shperiod, icgperiod and average, in that order)

        self.srv = self.create_service(RamanSpec, '/science/raman_spec_srv', self.raman_response)
        self.publisher_ = self.create_publisher(RamanSpectrum, '/science/raman_spec_msg', 10)

        self.continuous_mode = self.create_timer(0.2, self.continuous_callback)

    def continuous_callback(self):
        """
        Calls a single spectrum to be published
        """
        if self.is_continuous:
            msg_isvalid, msg_spectrum = RamanServer.get_spectrum(self.continuous_settings)
            self.publish_spectrum(msg_isvalid, msg_spectrum)

    def set_input(shperiod: int, icgperiod: int, singlecollectionmode: bool, average: int) -> List[int]:
        """
        Creates the array used for configuration to be sent to firmware.
        """
        result = np.zeros(12, np.uint8)

        #Transmit where in circular buffer to read from and to   
        result[0] = 69
        result[1] = 82

        # min is 20, max is 4294967295
        shperiodconverted = np.uint32(shperiod)
        result[2] = (shperiodconverted >> 24) & 0xff
        result[3] = (shperiodconverted >> 16) & 0xff
        result[4] = (shperiodconverted >> 8) & 0xff
        result[5] = shperiodconverted & 0xff

        # min is 14776, max is 4294967295
        icgperiodconverted = np.uint32(icgperiod)
        result[6] = (icgperiodconverted >> 24) & 0xff
        result[7] = (icgperiodconverted >> 16) & 0xff
        result[8] = (icgperiodconverted >> 8) & 0xff
        result[9] = icgperiodconverted & 0xff

        if singlecollectionmode:
            result[10] = 0
        else: # continuous collection mode
            result[10] = 1
        
        result[11] = average  # min is 1, max is 15

        return result
    
    def find_phase_end(output: List[int]) -> Tuple[bool, int]:
        """
        Finds the first occurrence of a phase signal (a flat peak in the spectrum that exceeds the PHASE_SIGNAL amount) in a given array
        """
        for element_index in range(len(output)):
            if output[element_index] > RamanServer.PHASE_SIGNAL:
                return True, output[element_index]
        
        return False, None
            
    def read_output_to_response(output: List[int]) -> List[int]:
        """
        Converts 8 bit integers read to 16 bit integers and balances the output (due to left and right sides of shift registers returning different values)
        """
        response = [0] * RamanServer.SPECTRA_SIZE

        # combining 8 bit integer pairs into respective 16 bit integer values
        for pixel in range(RamanServer.SPECTRA_SIZE):
            response[pixel] = (output[2*pixel+1] << 8) + output[2*pixel]
            
        # register has two sides which produce systematically differing values, so to reduce noise, an offset is applied to equal the values
        offset = 0
        for pixel in range(RamanServer.SPECTRA_SIZE):
            if pixel % 2 == 0:
                offset += response[pixel]
            else:
                offset -= response[pixel]
        offset = 2 * offset / RamanServer.SPECTRA_SIZE
	
        for pixel in range(RamanServer.SPECTRA_SIZE // 2):
            response[2*pixel] -= offset
	
        return response


    def raman_response(self, request, response):
        """
        Callback for a service request
        """
        if request.continuousendsignal:
            self.is_continuous = False
            self.get_logger().info("Continuous mode deactivated")
            response.continuousendedsignal = True
            return response
        
        response.continuousendedsignal = False

        if not request.singlecollectionmode:
            self.is_continuous = True
            self.get_logger().info("Continuous mode activated")
            self.continuous_settings = request.port, request.shperiod, request.icgperiod, request.average
            return response

        time_start = time.time()
        
        msg_isvalid, msg_spectrum = self.get_spectrum(request.port, request.shperiod, request.icgperiod, request.average)
        self.get_logger().info(f"Spectrum length is {len(msg_spectrum)}")
        self.publish_spectrum(msg_isvalid, msg_spectrum)

        time_taken = time.time() - time_start
        self.get_logger().info(f"Spectrum collection took {str(round(time_taken, 7))} seconds")

        return response

    def get_spectrum(self, serialport: str, shperiod: int, icgperiod: int, average: int) -> Tuple[bool, List[int]]:
        """
        Gets a single spectrum, returning a tuple of whether the spectrum is valid or not and the spectrum itself
        """
        try:
            ser = Serial(port=serialport, baudrate=RamanServer.BAUDRATE)

            #wait to clear the input and output buffers, if they're not empty data is corrupted
            while (ser.in_waiting > 0):
                ser.reset_input_buffer()
                ser.reset_output_buffer()
                time.sleep(0.01)

            input = RamanServer.set_input(shperiod, icgperiod, True, average)
            output = np.zeros(RamanServer.OUTPUT_SIZE, np.uint8)

            #transmit everything at once (the USB-firmware does not work if all bytes are not transmitted in one go)
            ser.write(input)
                
            #wait for the firmware to return data
            output = ser.read(RamanServer.OUTPUT_SIZE)

            ser.close()

            balanced_output = RamanServer.read_output_to_response(output)

            phase_end_found, phase_end_index = RamanServer.find_phase_end(balanced_output)

            if phase_end_found:
                final_output = balanced_output[0:phase_end_index]
                return True, final_output
               
        except SerialException as e:
            self.get_logger().error(f"Failed to process Raman service request: {str(e)}")

        return False, []

    def publish_spectrum(self, msg_isvalid: bool, msg_spectrum: List[int]) -> None:
        """
        Publishes RamanSpectrum message with inputted data
        """
        msg = RamanSpectrum()
        msg.isvalid = msg_isvalid
        msg.spectrum = msg_spectrum
        self.publisher_.publish(msg)


def main(args=None):
    rclpy.init(args=args)
    server = RamanServer()
    rclpy.spin(server)
    server.destroy_node()
    rclpy.shutdown()
    
if __name__ == "__main__":
    main()
