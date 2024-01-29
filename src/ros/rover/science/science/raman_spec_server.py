#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: ROS Node for publishing responses to service requests from GUI data for CCD data for Raman Spectra
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: raman_spec_server
TOPICS: None
SERVICES: 
    - /science/raman_spec [RamanSpectra] [Server]
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
        self.srv = self.create_service(RamanSpec, 'raman_spectra', self.raman_response)

    def raman_response(self, request, response):
        response.isvalid = False
        response.spectra = zeros(RamanServer.SPECTRA_SIZE, uint16)
        try:
            ser = Serial(request.port, RamanServer.BAUDRATE)

            #wait to clear the input and output buffers, if they're not empty data is corrupted
            while (ser.in_waiting > 0):
                ser.reset_input_buffer()
                ser.reset_output_buffer()
                sleep(0.01)

            input = zeros(12, uint8)
            output = zeros(RamanServer.OUTPUT_SIZE, uint8)

            #Transmit key 'ER' (firmware specific)   
            input[0] = 69
            input[1] = 82

            # min is 20, max is 4294967295
            shperiodconverted = uint32(request.shperiod)
            input[2] = (shperiodconverted >> 24) & 0xff
            input[3] = (shperiodconverted >> 16) & 0xff
            input[4] = (shperiodconverted >> 8) & 0xff
            input[5] = shperiodconverted & 0xff

            # min is 14776, max is 4294967295
            icgperiodconverted = uint32(request.icgperiod)
            input[6] = (icgperiodconverted >> 24) & 0xff
            input[7] = (icgperiodconverted >> 16) & 0xff
            input[8] = (icgperiodconverted >> 8) & 0xff
            input[9] = icgperiodconverted & 0xff

            input[10] = 0  # single collection mode only to fit one request -> one response format
            input[11] = uint8(request.average)  # min is 1, max is 15

            #transmit everything at once (the USB-firmware does not work if all bytes are not transmitted in one go)
            ser.write(input)
                
            #wait for the firmware to return data
            output = ser.read(RamanServer.OUTPUT_SIZE)

            ser.close()

            for pixel in range(RamanServer.SPECTRA_SIZE):
                response.spectra[pixel] = (output[2*pixel+1] << 8) + output[2*pixel]
           
            # register has two sides which produce systematically differing values, so to reduce noise, an offset is applied to equal the values
            offset = 0
            for pixel in range(RamanServer.SPECTRA_SIZE):
                if pixel % 2 == 0:
                    offset += response.spectra[pixel]
                else:
                    offset -= response.spectra[pixel]
            offset = 2 * offset / RamanServer.SPECTRA_SIZE

            for pixel in range(RamanServer.SPECTRA_SIZE / 2):
                response.spectra[2*pixel] -= offset 
            
            response.isvalid = True
            return response

        except SerialException:
            return response

def main():
    rclpy.init()
    server = RamanServer()
    rclpy.spin(server)
    server.destroy_node()
    rclpy.shutdown()
    
if __name__ == "__main__":
    main()
