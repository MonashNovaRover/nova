#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Read rover gps (skytraq) data from USB. Extract
relevant data and publish to network
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: SkytraqNode
TOPICS:
  - publisher: /electronics/gps_data [RoverPoseGPS]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	electronics
AUTHOR(S):	shelby n
CREATION:	25/02/2023
EDITED:		25/04/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - convert log to debug
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import serial


def cold_start(ser):
    cold_start_msg = 0xA1A0000F010300000101000000000000000000160D0A
    msg = msg_to_serial(cold_start_msg)
    ser.write(cold_start_msg)

def msg_to_serial(msg):
     return "b'" + str(msg) + "'"



def read_line(ser):
    txt = str(ser.read_until(b"$"))
    txt = txt.rstrip("\\r\\n$'")
    txt = txt.lstrip("b'")
    print(txt)


def config_port(port_name, baud):
    ser = serial.Serial()

    ser.baudrate = baud

    if port_name == "":
        port_name = "/dev/ttyUSB0"
    ser.port = port_name

    ser.open()

    return ser



if __name__ == "__main__":
    ser = config_port("COM3", 115200)
    while 1:
        for i in range(1000):
            read_line(ser)

        cold_start(ser)


