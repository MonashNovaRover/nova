#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This node monitors the health of the radio systems.
Returns signal strength (dB), bandwidth used (kbits sent
and received) and ping (ms)
Requires radio and destination IPs and ethernet interface
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: radio_monitor
TOPICS:
  - /electronics/radio_status       [RadioStatus]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	electronics
AUTHOR(S):	Emily Kuo
CREATION:	22/09/2021
EDITED:		17/12/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
If you have issues connecting to the radio device
    due to SSH connections, run the following line
    in the Terminal:
        ssh-copy-id ***REMOVED***@192.168.1.201
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

# Include all relevant packages
import rclpy
from rclpy.node import Node
from rclpy.timer import Rate
from core.msg import RadioStatus
from fabric import Connection
import re, time, psutil, subprocess, sys

# Stores the device information for each of the device settings
DEVICE_INFO = {

    # Old Rocket Systems
    "ROCKET": {
        "ROS": True,                        # Using ROS or not
        "dest_IP":      "192.168.1.204",    # Destination IP of Jetson
        "radio_IP":     "192.168.3.155",    # Base Station Radio IP
        "host":         "nova",             # Host Name of the Radio
        "password":     "***REMOVED***",         # Password of Radio device
        "interface":    "enp3s0f1",         # Ethernet Interface 
    },

    # New Bullet Systems
    "BULLET": {
        "ROS": True,                        # Using ROS or not
        "dest_IP":      "192.168.1.204",    # Destination IP of Jetson
        "radio_IP":     "192.168.1.201",    # Base Station Radio IP
        "host":         "***REMOVED***",   # Host Name of the Radio
        "password":     "***REMOVED***",   # Password of Radio device
        "interface":    "enp3s0f1",         # Ethernet Interface 
    },
}

# Default device
DEVICE_DEFAULT = "BULLET"

# Main radio monitor class
class RadioMonitor(Node):
    
    def __init__(self, device):
        
        # Print initialisation information
        print("Initialising Radio Monitor class for %s" % device)

        # Get the device info
        self.is_ros =       DEVICE_INFO[device]["ROS"]
        self.dest_IP =      DEVICE_INFO[device]["dest_IP"]
        self.radio_IP =     DEVICE_INFO[device]["radio_IP"]
        self.password =     DEVICE_INFO[device]["password"]
        self.interface =    DEVICE_INFO[device]["interface"]
        self.host =         DEVICE_INFO[device]["host"]

        self.command = "ping -c 3 -W 1 " + self.dest_IP #shell command
        self.ssh_connection = Connection(host=self.host + "@" + self.radio_IP, connect_kwargs={"password":self.password}, connect_timeout=3)
        
        if self.is_ros:
            super().__init__("radio_monitor")
            self.publisher = self.create_publisher(RadioStatus, "/electronics/radio_status", 10)


    def connect_to_radio(self):
        '''
        Initiates ssh connection to the radio
        '''
        print("Connecting to radio")
        self.ssh_connection.open()
  

    def loop_function(self):
        '''
        Main function
        Runs all the radio monitor commands
        Prints/ publishes the output
        '''
        sent, recv = self.get_bandwidth()
        ping = self.get_ping()
        signal = self.get_signal()

        # Print the data
	#        print("Signal: %ddb, \tSent: %dkb, \tRecv: %dkb, \tPing: %dms" % (signal, sent, recv, ping))

        # Publish data over ROS
        if self.is_ros:
            radio_msg = RadioStatus()
            radio_msg.signal = int(signal)
            radio_msg.sent = int(sent)
            radio_msg.recv = int(recv)
            radio_msg.ping = int(ping)
            self.publisher.publish(radio_msg)
  

    def get_bandwidth(self):
        '''
        Monitors data transfer over the ethernet port over one second
        Returns sent, received bandwidth (kbits)
        '''
        sent_old = psutil.net_io_counters(pernic=True)[self.interface][0]
        recv_old = psutil.net_io_counters(pernic=True)[self.interface][1]
        time.sleep(1)
        sent_new = psutil.net_io_counters(pernic=True)[self.interface][0]
        recv_new = psutil.net_io_counters(pernic=True)[self.interface][1]

        sent = ((sent_new-sent_old) * 8)/1024 #converts to kbits
        recv = ((recv_new-recv_old) * 8)/1024 

        return sent, recv


    def get_signal(self):
        '''
        Sshs into the base station radio to monitor connection to the rover
        Returns signal strength (dB) 
        -96dB indicates that the two radios are disconnected
        '''
        raw_output = self.ssh_connection.run('mca-status | grep -Ei "signal"', hide=True)
        msg = "{0.stdout}"
        signal_string = msg.format(raw_output)
        initial_split = signal_string.split('=')
        signal_split = initial_split[1].split('\r')[0]
        signal = int(signal_split)

        return signal


    def get_ping(self) -> int:
        '''
        Pings destination IP three times and averages the result
        Returns ping (ms)
        '''
        try: 
            output = subprocess.check_output(self.command, shell=True)
            matches = re.findall(b" time=([\d.]+) ms", output)
            matches = [float(match) for match in matches]
            ms = sum(matches)/len(matches)

        # Returns this if there is a ping error
        except: 
            ms=9999
        
        return ms


# Main function sets up the ROS class
def main():

    # Grab parameter and check if not existed
    if len(sys.argv) <= 1:
        device = DEVICE_DEFAULT

    # Otherwise take the parameter
    else:
        
        # If found an invalid device
        if sys.argv[1].upper() not in DEVICE_INFO.keys():
            print("Invalid device. Using default device.")
            device = DEVICE_DEFAULT
        
        # Else set the device
        else:
            device = sys.argv[1].upper()

    # Run the monitor code
    rclpy.init()
    radio_monitor = RadioMonitor(device)

    # Attempt to run the radio monitor code
    try:
        # Connect to the radio
        radio_monitor.connect_to_radio()

        # Forevery loop
        while True:
            radio_monitor.loop_function()
    
    # If issues found, raise exception
    except Exception as e:
        print("An error occurred with the radio monitor.")
        print(e)


# Called when the script executes
if __name__=="__main__":
    main()
