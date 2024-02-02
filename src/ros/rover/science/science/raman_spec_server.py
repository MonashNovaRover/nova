#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: ROS Node for publishing responses to service requests from GUI data for CCD data for Raman Spectra
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: raman_spec_server
TOPICS: 
    - /science/raman_spec_stream [RamanSpectraStream] [Publisher]
SERVICES: 
    - /science/raman_spec_srv [RamanSpec] [Server]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: science
SOURCE AUTHOR:	Esben Rossel <esbenrossel@gmail.com>
AUTHOR: Connor Macdougall
CREATION:	18/01/2024
EDITED:		25/01/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
MORE INFO:
 - https://www.notion.so/Raman-Spectra-0161f5611e934a779247f3733ca8a608
"""

import rclpy
from core.srv import RamanSpec
from numpy import zeros, uint8, uint16, uint32
from serial import Serial, SerialException
from time import sleep



class RamanServer(rclpy.node.Node):
    # Constants set by firmware/hardware of STM32F103
    BAUDRATE = 115200
    MASTERCLOCK = 800000
    SPECTRA_SIZE = 3694
    OUTPUT_SIZE = 7388

    def __init__(self):
        super().__init__('raman_spec_server')
        self.srv = self.create_service(RamanSpec, '/science/raman_spec_srv', self.raman_response)
        self.continuous_mode = False

    def get_continuous_mode(self):
        return self.continuous_mode

    def set_input(shperiod, icgperiod, singlecollectionmode, average):
        result = zeros(12, uint8)

        #Transmit key 'ER' (firmware specific)   
        result[0] = 69
        result[1] = 82

        # min is 20, max is 4294967295
        shperiodconverted = uint32(shperiod)
        result[2] = (shperiodconverted >> 24) & 0xff
        result[3] = (shperiodconverted >> 16) & 0xff
        result[4] = (shperiodconverted >> 8) & 0xff
        result[5] = shperiodconverted & 0xff

        # min is 14776, max is 4294967295
        icgperiodconverted = uint32(icgperiod)
        result[6] = (icgperiodconverted >> 24) & 0xff
        result[7] = (icgperiodconverted >> 16) & 0xff
        result[8] = (icgperiodconverted >> 8) & 0xff
        result[9] = icgperiodconverted & 0xff

        if singlecollectionmode:
            result[10] = 0
        else: # continuous collection mode
            result[10] = 1
        
        result[11] = uint8(average)  # min is 1, max is 15

        return result

    
    def read_output_to_response(output):
        response = zeros(RamanServer.SPECTRA_SIZE, uint16)

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

        return response



    def raman_response(self, request, response):
        response.isvalid = False
        response.spectra = zeros(RamanServer.SPECTRA_SIZE, uint16)
        if request.singlecollectionmode:
            try:
                ser = Serial(request.port, RamanServer.BAUDRATE)

                #wait to clear the input and output buffers, if they're not empty data is corrupted
                while (ser.in_waiting > 0):
                    ser.reset_input_buffer()
                    ser.reset_output_buffer()
                    sleep(0.01)

                input = RamanServer.set_input(request.shperiod, request.icgperiod, request.singlecollectionmode, request.average)
                output = zeros(RamanServer.OUTPUT_SIZE, uint8)

                #transmit everything at once (the USB-firmware does not work if all bytes are not transmitted in one go)
                ser.write(input)
                    
                #wait for the firmware to return data
                output = ser.read(RamanServer.OUTPUT_SIZE)

                ser.close()

                response.spectra = RamanServer.read_output_to_response(output)
                response.isvalid = True
                
                return response

            except SerialException:
                return response
        elif request.continuousendsignal:
            self.continuous_mode = False
        else:
            pass

def main():
    rclpy.init()
    server = RamanServer()
    rclpy.spin(server)
    server.destroy_node()
    rclpy.shutdown()
    
if __name__ == "__main__":
    main()
