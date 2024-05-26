#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: ROS Node for publishing + serving responses to service
requests from GUI data for CCD data for Raman Spectra
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
EDITED:		    22/05/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Fix getting spectra out of phase

MORE INFO:
 - https://www.notion.so/Raman-Spectra-0161f5611e934a779247f3733ca8a608
"""

import logging
import rclpy
from rclpy.node import Node

from nova_interfaces.msg import RamanSpectrum
from nova_interfaces.srv import RamanSpec

import numpy as np
from serial import Serial, SerialException
import time
from typing import Tuple, List

class RamanSpecServer(Node):
    # Constants set by firmware/hardware of STM32F103
    BAUDRATE = 115200
    MASTERCLOCK = 800000
    INITIAL_SPECTRA_SIZE = 3694                         # This a ROS param then decrease it during testing until you only get one phase perfectly (if phase length does not change)
    OUTPUT_SIZE = 2 * INITIAL_SPECTRA_SIZE
    CIRCULAR_BUFFER_START = ord('E')
    CIRCULAR_BUFFER_END = ord('R')
    SINGLE_COLLECTION_MODE = 0
    CONTINUOUS_COLLECTION_MODE = 1

    # Factors for spectrum collection
    PHASE_SIGNAL = 3730
    MINIMUM_PHASE_LENGTH = 1500
    SPECTRUM_CROP = 32  # the number of pixels after phase signal ends to ignore
    LOOPS_FOR_SINGLE_COLLECTION = 1

    # ROS spec channels
    SPEC_SERVICE = '/science/raman_spec_srv'
    SPEC_TOPIC = '/science/raman_spec_msg'

    DEFAULT_CONTINUOUS_SETTINGS = None, None, None, None    # A tuple of 4 values (port, shperiod, icgperiod and average, in that order)


    def __init__(self):
        super().__init__('raman_spec_server')
        self.get_logger().set_level(logging.INFO)
        self.get_logger().info("Raman Spec Server starting")


        # initialising node values
        self.is_continuous = False
        self.continuous_settings = RamanSpecServer.DEFAULT_CONTINUOUS_SETTINGS  
        
        # for spectrum
        self.spec_srv = self.create_service(RamanSpec, RamanSpecServer.SPEC_SERVICE, self.raman_spec_response)
        self.spec_publisher_ = self.create_publisher(RamanSpectrum, RamanSpecServer.SPEC_TOPIC, 10)
        self.timer_continuous_mode = self.create_timer(2, self.continuous_spec_callback)

        self.declare_parameter('raman_spectrum_length', RamanSpecServer.INITIAL_SPECTRA_SIZE)


    def continuous_spec_callback(self):
        """
        Calls a single spectrum to be published
        """
        if self.is_continuous:
            self.publish_spectrum(self.get_valid_spectrum(self.continuous_settings))


    def set_spec_input(shperiod: int, icgperiod: int, singlecollectionmode: bool, average: int) -> List[int]:
        """
        Creates the array used for configuration to be sent to firmware.
        """
        result = np.zeros(12, np.uint8)

        #Transmit where in circular buffer to read from and to   
        result[0] = RamanSpecServer.CIRCULAR_BUFFER_START
        result[1] = RamanSpecServer.CIRCULAR_BUFFER_END

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
            result[10] = RamanSpecServer.SINGLE_COLLECTION_MODE
        else: # continuous collection mode
            result[10] = RamanSpecServer.CONTINUOUS_COLLECTION_MODE
        
        result[11] = average  # min is 1, max is 15

        return result


    def find_full_phase(spectrum: List[int]) -> Tuple[int, int]:
        """
        For finding peaks through analysis
        """
        spectrum_start = None
        spectrum_end = None

        if len(spectrum) == 0:
            return spectrum_start, spectrum_end

        for start_of_first_phase_signal in range(len(spectrum)):
            if spectrum[start_of_first_phase_signal] > RamanSpecServer.PHASE_SIGNAL:
                break

        for end_of_first_phase_signal in range(start_of_first_phase_signal, len(spectrum)):
            if spectrum[end_of_first_phase_signal] < RamanSpecServer.PHASE_SIGNAL and (end_of_first_phase_signal + 5 < len(spectrum) or spectrum[end_of_first_phase_signal + 5] < RamanSpecServer.PHASE_SIGNAL):
                spectrum_start = end_of_first_phase_signal + RamanSpecServer.SPECTRUM_CROP
                break
        
        if end_of_first_phase_signal + RamanSpecServer.MINIMUM_PHASE_LENGTH < len(spectrum):
            for start_of_second_phase_signal in range(end_of_first_phase_signal + RamanSpecServer.MINIMUM_PHASE_LENGTH, len(spectrum)):
                if spectrum[start_of_second_phase_signal] > RamanSpecServer.PHASE_SIGNAL:
                    spectrum_end = start_of_second_phase_signal - RamanSpecServer.SPECTRUM_CROP
                    break

        return spectrum_start, spectrum_end


    def read_output_to_response(self, output: List[int]) -> List[int]:
        """
        Converts 8 bit integers read to 16 bit integers and balances the output (due to left and right sides of shift registers returning different values)
        """
        current_spectra_size = self.get_parameter('raman_spectrum_length')
        response = [0] * current_spectra_size

        # combining 8 bit integer pairs into respective 16 bit integer values
        for pixel in range(current_spectra_size):
            response[pixel] = (output[2*pixel+1] << 8) + output[2*pixel]
            
        # register has two sides which produce systematically differing values, so to reduce noise, an offset is applied to equal the values
        offset = 0
        for pixel in range(current_spectra_size):
            if pixel % 2 == 0:
                offset += response[pixel]
            else:
                offset -= response[pixel]
        offset = 2 * offset // current_spectra_size
	
        for pixel in range(current_spectra_size // 2):
            if response[2*pixel] > offset:
                response[2*pixel] -= offset
            else:
                response[2*pixel] = 0
	
        return response

    
    def get_valid_spectrum(self, port, shperiod, icgperiod, average):
        """
        Returns a Tuple of True, and single spectrum that has a full phase, or False, and an empty list if a single spectrum that has a full phase is not found
        """
        spectrum_start = None
        spectrum_end = None
        loop_count = 0
        while loop_count < RamanSpecServer.LOOPS_FOR_SINGLE_COLLECTION: # and not spectrum_end: 
            spectrum = self.get_spectrum(port, shperiod, icgperiod, average)
            # spectrum_start, spectrum_end = RamanSpecServer.find_full_phase(spectrum)
            self.get_logger().info(f"Spectrum start is {spectrum_start}")
            loop_count += 1
        
        return True, spectrum


    def raman_spec_response(self, request, response):
        """
        Callback for a service request
        """
        self.get_logger().info("Request received")
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
        
        isvalid, spectrum = self.get_valid_spectrum(request.port, request.shperiod, request.icgperiod, request.average)
        self.publish_spectrum(isvalid, spectrum)

        time_taken = time.time() - time_start
        self.get_logger().info(f"Spectrum collection took {str(round(time_taken, 7))} seconds")

        return response


    def get_spectrum(self, serialport: str, shperiod: int, icgperiod: int, average: int) -> List[int]:
        """
        Gets a single spectrum, returning the spectrum itself
        """
        try:
            ser = Serial(port=serialport, baudrate=RamanSpecServer.BAUDRATE)

            current_spectra_size = self.get_parameter('raman_spectrum_length')

            #wait to clear the input and output buffers, if they're not empty data is corrupted
            while (ser.in_waiting > 0 or ser.out_waiting > 0):
                ser.reset_input_buffer()
                ser.reset_output_buffer()
                time.sleep(0.01)

            input = RamanSpecServer.set_spec_input(shperiod, icgperiod, True, average)
            output = np.zeros(2*current_spectra_size, np.uint8)

            #transmit everything at once (the USB-firmware does not work if all bytes are not transmitted in one go)
            ser.write(input)
                
            #wait for the firmware to return data
            output = ser.read(2*current_spectra_size)

            ser.close()

            return self.read_output_to_response(output)
               
        except SerialException as e:
            self.get_logger().error(f"Failed to process Raman service request: {str(e)}")
            return []


    def publish_spectrum(self, msg_isvalid, msg_spectrum: List[int]) -> None:
        """
        Publishes RamanSpectrum message with inputted data
        """
        msg = RamanSpectrum()
        msg.isvalid = msg_isvalid
        msg.spectrum = msg_spectrum
        self.spec_publisher_.publish(msg)


def main(args=None):
    rclpy.init(args=args)
    server = RamanSpecServer()
    rclpy.spin(server)
    server.destroy_node()
    rclpy.shutdown()
    
if __name__ == "__main__":
    main()
