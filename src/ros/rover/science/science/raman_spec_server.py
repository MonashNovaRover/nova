#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: ROS Node for publishing responses to service
requests from GUI data for CCD data for Raman Spectra
and maintaining/updating Raman mechanical state
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: raman_spec_server
TOPICS: 
    - /science/raman_spec_msg [RamanSpectrum] [Publisher]
    - /science/raman_mech_msg [RamanState] [Publisher]
SERVICES: 
    - /science/raman_spec_srv [RamanSpec] [Server]
    - /science/raman_mech_srv [RamanMech] [Server]
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
import jcan

from nova_interfaces.msg import RamanSpectrum, RamanState
from nova_interfaces.srv import RamanMech, RamanSpec

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
    CIRCULAR_BUFFER_START = 69
    CIRCULAR_BUFFER_END = 82
    SINGLE_COLLECTION_MODE = 0
    CONTINUOUS_COLLECTION_MODE = 1

    # Factors for spectrum collection
    PHASE_SIGNAL = 3675
    RESOLUTION_REDUCING_FACTOR = 10
    MINIMUM_PHASE_LENGTH = 1500
    SPECTRUM_CROP = 30  # the number of pixels after phase signal ends to ignore
    LOOPS_FOR_SINGLE_COLLECTION = 5

    # CAN commands
    CARD_ID = 0x0A0
    DISABLE_CARD = 0x00
    GREEN_LASER_ID = 0x01
    RED_LASER_ID = 0x02
    MIRROR_SERVO_ID = 0x03
    FILTER_SERVO_ID = 0x04
    STEPPER_ID = 0x05
    CAN_COMMAND_ON = 0x01
    CAN_COMMAND_OFF = 0x00

    # ROS params
    CAN_BUS_PARAM = "can_bus"

    # ROS spec channels
    SPEC_SERVICE = '/science/raman_spec_srv'
    SPEC_TOPIC = '/science/raman_spec_msg'

    # ROS mech channels
    MECH_STATE_SERVICE = '/science/raman_mech_srv'
    MECH_STATE_TOPIC = '/science/raman_mech_msg'

    # initial state values
    DEFAULT_MECH = False, False, 0.0, 0.0, 0.0             # A tuple of 5 values (greenlaseron, redlaseron, filterselection, steppervalue and mirrorservo, in that order)
    DEFAULT_CONTINUOUS_SETTINGS = None, None, None, None    # A tuple of 4 values (port, shperiod, icgperiod and average, in that order)


    def __init__(self):
        super().__init__('raman_spec_server')
        self.get_logger().set_level(logging.INFO)
        self.get_logger().info("Raman Spec Server starting")

        self.declare_parameter(RamanServer.CAN_BUS_PARAM, "can1")

        # initialising node values
        self.is_continuous = False
        self.continuous_settings = RamanServer.DEFAULT_CONTINUOUS_SETTINGS  
        self.mech_state = RamanServer.DEFAULT_MECH
        
        # for spectrum
        self.spec_srv = self.create_service(RamanSpec, RamanServer.SPEC_SERVICE, self.raman_spec_response)
        self.spec_publisher_ = self.create_publisher(RamanSpectrum, RamanServer.SPEC_TOPIC, 10)
        self.timer_continuous_mode = self.create_timer(0.5, self.continuous_spec_callback)

        # for mechanical
        self.mech_srv = self.create_service(RamanMech, RamanServer.MECH_STATE_SERVICE, self.raman_mech_response)
        self.mech_publisher_ = self.create_publisher(RamanState, RamanServer.MECH_STATE_TOPIC, 10)
        self.timer_publish_state = self.create_timer(1, self.publish_state)
        self.timer_send_can_commands = self.create_timer(0.2, self.send_can_commands)

        # for CAN commands
        self.bus = jcan.Bus()
        self.bus.open(self.get_parameter(self.CAN_BUS_PARAM).value)
        self.timer_spin_can = self.create_timer(0.05, self.bus.spin)


    def continuous_spec_callback(self):
        """
        Calls a single spectrum to be published
        """
        if self.is_continuous:
            self.publish_spectrum(self.get_valid_spectrum(self.continuous_settings))


    def send_can_commands(self):
        """
        Sends all can commands to update mechanical state to what is current 
        """
        self.send_laser_command()
        self.send_filter_command()
        self.send_stepper_command()
        self.send_mirror_command()

    def send_can_command(self, commands):
        for command in commands:
            mech_frame = jcan.Frame(RamanServer.CARD_ID, command)
            self.bus.send(mech_frame)


    def send_laser_command(self):
        if self.mech_state[0]:
            self.send_can_command([[RamanServer.GREEN_LASER_ID, RamanServer.CAN_COMMAND_ON], [RamanServer.RED_LASER_ID, RamanServer.CAN_COMMAND_OFF]])
        elif self.mech_state[1]:
            self.send_can_command([[RamanServer.GREEN_LASER_ID, RamanServer.CAN_COMMAND_OFF], [RamanServer.RED_LASER_ID, RamanServer.CAN_COMMAND_ON]])
        else:
            self.send_can_command([[RamanServer.GREEN_LASER_ID, RamanServer.CAN_COMMAND_OFF], [RamanServer.RED_LASER_ID, RamanServer.CAN_COMMAND_OFF]])

    def send_filter_command(self):
        self.send_can_command([[RamanServer.FILTER_SERVO_ID, self.mech_state[2]]])

    def send_stepper_command(self):
        self.send_can_command([[RamanServer.STEPPER_ID, self.mech_state[3]]])

    def send_mirror_command(self):
        self.send_can_command([[RamanServer.MIRROR_SERVO_ID, self.mech_state[4]]]) 


    def set_spec_input(shperiod: int, icgperiod: int, singlecollectionmode: bool, average: int) -> List[int]:
        """
        Creates the array used for configuration to be sent to firmware.
        """
        result = np.zeros(12, np.uint8)

        #Transmit where in circular buffer to read from and to   
        result[0] = RamanServer.CIRCULAR_BUFFER_START
        result[1] = RamanServer.CIRCULAR_BUFFER_END

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
            result[10] = RamanServer.SINGLE_COLLECTION_MODE
        else: # continuous collection mode
            result[10] = RamanServer.CONTINUOUS_COLLECTION_MODE
        
        result[11] = average  # min is 1, max is 15

        return result


    def find_full_phase(spectrum: List[int]) -> Tuple[int, int]:
        spectrum_start = None
        spectrum_end = None

        if len(spectrum) == 0:
            return spectrum_start, spectrum_end

        for start_of_first_phase_signal in range(len(spectrum)):
            if spectrum[start_of_first_phase_signal] > RamanServer.PHASE_SIGNAL:
                break

        for end_of_first_phase_signal in range(start_of_first_phase_signal, len(spectrum)):
            if spectrum[end_of_first_phase_signal] < RamanServer.PHASE_SIGNAL and spectrum[end_of_first_phase_signal + 5] < RamanServer.PHASE_SIGNAL:
                spectrum_start = end_of_first_phase_signal + RamanServer.SPECTRUM_CROP
                break
        
        if end_of_first_phase_signal + RamanServer.MINIMUM_PHASE_LENGTH < len(spectrum):
            for start_of_second_phase_signal in range(end_of_first_phase_signal + RamanServer.MINIMUM_PHASE_LENGTH, len(spectrum)):
                if spectrum[start_of_second_phase_signal] > RamanServer.PHASE_SIGNAL:
                    spectrum_end = start_of_second_phase_signal - RamanServer.SPECTRUM_CROP
                    break

        return spectrum_start, spectrum_end


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
        offset = 2 * offset // RamanServer.SPECTRA_SIZE
	
        for pixel in range(RamanServer.SPECTRA_SIZE // 2):
            if response[2*pixel] > offset:
                response[2*pixel] -= offset
            else:
                response[2*pixel] = 0
	
        return response


    def raman_mech_response(self, request, response):
        """
        Updates the mechanical state that the node will send can commands based on
        """
        self.mech_state = request.green_laser_on, request.red_laser_on, request.filter_selection, request.stepper_value, request.mirror_servo
        response.success = True
        return response


    def publish_state(self):
        """
        Publishes raman mechanical state
        """
        msg = RamanState()
        msg.green_laser_on, msg.red_laser_on, msg.filter_selection, msg.stepper_value, msg.mirror_servo = self.mech_state
        self.mech_publisher_.publish(msg)

    
    def get_valid_spectrum(self, port, shperiod, icgperiod, average):
        """
        Returns a Tuple of True, and single spectrum that has a full phase, or False, and an empty list if a single spectrum that has a full phase is not found
        """
        spectrum_start = None
        spectrum_end = None
        loop_count = 0
        while loop_count < RamanServer.LOOPS_FOR_SINGLE_COLLECTION and not spectrum_end: 
            spectrum = self.get_spectrum(port, shperiod, icgperiod, average)
            spectrum_start, spectrum_end = RamanServer.find_full_phase(spectrum)
            self.get_logger().info(f"Spectrum length is {len(spectrum[spectrum_start:spectrum_end])} and with reduction is {len(spectrum[spectrum_start:spectrum_end:RamanServer.RESOLUTION_REDUCING_FACTOR])}")
            loop_count += 1
        
        if spectrum_end:
            return True, spectrum[spectrum_start:spectrum_end:RamanServer.RESOLUTION_REDUCING_FACTOR]
        else:
            return False, []


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
        
        self.publish_spectrum(self.get_valid_spectrum(request.port, request.shperiod, request.icgperiod, request.average))

        time_taken = time.time() - time_start
        self.get_logger().info(f"Spectrum collection took {str(round(time_taken, 7))} seconds")

        return response


    def get_spectrum(self, serialport: str, shperiod: int, icgperiod: int, average: int) -> List[int]:
        """
        Gets a single spectrum, returning the spectrum itself
        """
        try:
            ser = Serial(port=serialport, baudrate=RamanServer.BAUDRATE)

            #wait to clear the input and output buffers, if they're not empty data is corrupted
            while (ser.in_waiting > 0):
                ser.reset_input_buffer()
                ser.reset_output_buffer()
                time.sleep(0.01)

            input = RamanServer.set_spec_input(shperiod, icgperiod, True, average)
            output = np.zeros(RamanServer.OUTPUT_SIZE, np.uint8)

            #transmit everything at once (the USB-firmware does not work if all bytes are not transmitted in one go)
            ser.write(input)
                
            #wait for the firmware to return data
            output = ser.read(RamanServer.OUTPUT_SIZE)

            ser.close()

            return RamanServer.read_output_to_response(output)
               
        except SerialException as e:
            self.get_logger().error(f"Failed to process Raman service request: {str(e)}")
            return []


    def publish_spectrum(self, msg_spectrum: List[int]) -> None:
        """
        Publishes RamanSpectrum message with inputted data
        """
        msg = RamanSpectrum()
        msg.isvalid = True
        msg.spectrum = msg_spectrum
        self.spec_publisher_.publish(msg)


def main(args=None):
    rclpy.init(args=args)
    server = RamanServer()
    rclpy.spin(server)
    server.destroy_node()
    rclpy.shutdown()
    
if __name__ == "__main__":
    main()
