#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: ROS Node for publishing responses to service requests from GUI data for CCD data for Raman Spectra
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: raman_spec_responder
TOPICS: None
SERVICES: 
    - /science/raman_spec [RamanSpectra] [Server]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:
AUTHOR(S):	Connor Macdougall
CREATION:	18/01/2024
EDITED:		18/01/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - add balance output code

MORE INFO:
 - https://www.notion.so/Raman-Spectra-0161f5611e934a779247f3733ca8a608
"""

from rclpy.node import Node
from core.srv import RamanSpec
import numpy as np


class RamanServer(Node):

    def __init__(self):
        super().__init__('RamanServer')
        self.srv = self.create_service(RamanSpec, 'raman_spectra', self.raman_response)

    def raman_response(self, request, response):
        input = np.zeros(12, np.uint8)

        #Transmit key 'ER'    
        input[0] = 69
        input[1] = 82
        input[2] = (request.shperiod >> 24) & 0xff
        input[3] = (request.shperiod >> 16) & 0xff
        input[4] = (request.shperiod >> 8) & 0xff
        input[5] = request.shperiod & 0xff
        input[6] = (request.icgperiod >> 24) & 0xff
        input[7] = (request.icgperiod >> 16) & 0xff
        input[8] = (request.icgperiod >> 8) & 0xff
        input[9] = request.icgperiod & 0xff
        input[10] = 0  # single collection mode only to fit one request -> one response format
        input[11] = request.average

        return response