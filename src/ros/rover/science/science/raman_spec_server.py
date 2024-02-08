#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: ROS Node for publishing responses to service requests from GUI data for CCD data for Raman Spectra
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: raman_spec_server
TOPICS: 
    - /science/raman_spec_msg [RamanSpectraStream] [Publisher]
SERVICES: 
    - /science/raman_spec_srv [RamanSpec] [Server]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: science
SOURCE AUTHOR:	Esben Rossel <esbenrossel@gmail.com>
AUTHOR: Connor Macdougall
CREATION:	18/01/2024
EDITED:		07/02/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
MORE INFO:
 - https://www.notion.so/Raman-Spectra-0161f5611e934a779247f3733ca8a608
"""

from math import ceil
import rclpy
from rclpy.node import Node
from core.srv import RamanSpec
from core.msg import RamanSpectrum
import numpy as np
from serial import Serial, SerialException
from time import sleep


class RamanServer(Node):
    # Constants set by firmware/hardware of STM32F103
    BAUDRATE = 115200
    MASTERCLOCK = 800000
    SPECTRA_SIZE = 3694
    OUTPUT_SIZE = 7388

    def __init__(self):
        super().__init__('raman_spec_server')
        self.srv = self.create_service(RamanSpec, '/science/raman_spec_srv', self.raman_response)
        self.publisher_ = self.create_publisher(RamanSpectrum, '/science/raman_spec_msg', 10)
        self.continuous_mode = False

    def is_in_continuous_mode(self):
        return self.continuous_mode

    def set_input(shperiod, icgperiod, singlecollectionmode, average):
        result = np.zeros(12, np.uint8)

        #Transmit key 'ER' (firmware specific)   
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
            result[10] = np.uint8(0)
        else: # continuous collection mode
            result[10] = np.uint8(1)
        
        result[11] = np.uint8(average)  # min is 1, max is 15

        return result

    def reduce_resolution_by_a_factor_of(factor, input):
        if factor == 1:
            return input
        reduced_result = [0]* (RamanServer.SPECTRA_SIZE // factor)
        for reduced_index in range(len(reduced_result)):
            sum = 0
            count_at_final_index = None
            for pixel in range(factor):
                try:
                    sum += input[reduced_index + pixel]
                except IndexError:
                    count_at_final_element = pixel
                    break
            if count_at_final_index:
                reduced_result[reduced_index] = sum / count_at_final_element
            else:
                reduced_result[reduced_index] = sum / factor
        return reduced_result
            
            


    
    def read_output_to_response(output):
        response = np.zeros(RamanServer.SPECTRA_SIZE, np.uint16)

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
	
        for pixel in range(RamanServer.SPECTRA_SIZE / 2):
            response[2*pixel] -= offset
	
        return response.tolist()



    def raman_response(self, request, response):
        msg = RamanSpectrum()
        msg.isvalid = False
        response.continuousendedsignal = False
        try:
            if request.continuousendsignal:
                issinglecollection = True
                self.continuous_mode = False
            else:
                issinglecollection = request.singlecollectionmode

            ser = Serial(port=str(request.port), baudrate=RamanServer.BAUDRATE, timeout=1)

            #wait to clear the input and output buffers, if they're not empty data is corrupted
            while (ser.in_waiting > 0):
                ser.reset_input_buffer()
                ser.reset_output_buffer()
                sleep(0.01)

            input = RamanServer.set_input(request.shperiod, request.icgperiod, issinglecollection, request.average)
            output = np.zeros(RamanServer.OUTPUT_SIZE, np.uint8)

            #transmit everything at once (the USB-firmware does not work if all bytes are not transmitted in one go)
            ser.write(input)

            if not issinglecollection:
                self.continuous_mode = True

                #loop to acquire and send data continuously
                while self.is_in_continuous_mode():
                    output = ser.read(RamanServer.OUTPUT_SIZE)
                    
                    full_res_data = RamanServer.read_output_to_response(output)

                    msg.isvalid = True
                    msg.spectrum = RamanServer.reduce_resolution_by_a_factor_of(response.resolutionreductionfactor, full_res_data).tolist()
                    self.publisher_.publish(msg)
                
                response.continuousendedsignal = True

                #resend settings with continuous transmission disabled to avoid flooding of the serial port
                input = RamanServer.set_input(request.shperiod, request.icgperiod, True, request.average)

                #transmit everything at once (the USB-firmware does not work if all bytes are not transmitted in one go)
                ser.write(input)
                
            #wait for the firmware to return data
            output = ser.read(RamanServer.OUTPUT_SIZE)

            ser.close()

            full_res_result = RamanServer.read_output_to_response(output)
            msg.isvalid = True
            msg.spectrum = RamanServer.reduce_resolution_by_a_factor_of(response.resolutionreductionfactor, full_res_result).tolist()
            self.publisher_.publish(msg)

            return response
	
        except SerialException:
            msg.spectrum = [0, 12, 7, 9, 4, 3, 1]
            msg.isvalid = True
            self.publisher_.publish(msg)
            return response

def main(args=None):
    rclpy.init(args=args)
    server = RamanServer()
    rclpy.spin(server)
    server.destroy_node()
    rclpy.shutdown()
    
if __name__ == "__main__":
    main()
