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
  - /chassis/radio_status       [RadioStatus]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	electronics
AUTHOR(S):	Emily Kuo
CREATION:	22/09/2021
EDITED:		11/12/2025
EDITED BY:  Binuda Kalugalage 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
If you have issues connecting to the radio device
    due to SSH connections, run the following line
    in the Terminal:
        ssh-copy-id ***REMOVED***@10.0.1.11
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

# Include all relevant packages
import rclpy
from rclpy.node import Node
from nova_interfaces.msg import RadioStatus
from fabric import Connection
import re, time, psutil, subprocess
import os

# Main radio monitor class
class RadioMonitor(Node):

    # Parameters
    DEVICE_PARAM = "device"
    DEST_IP_PARAM = "dest_ip"
    BASE_IP_PARAM = "base_ip"
    HOST_PARAM = "host"
    PASSWORD_PATH_PARAM = "password_path"
    INTERFACE_PARAM = "interface"
    
    def __init__(self):
        super().__init__("radio_monitor")

        # Get device
        self.declare_parameter(self.DEVICE_PARAM, "BULLET")
        self.device = self.get_parameter(self.DEVICE_PARAM).value
        self.device_prefix = f"devices.{self.device}"

        # Declare parameters
        self.declare_parameters(
            namespace=self.device_prefix,
            parameters=[
                (self.DEST_IP_PARAM, "10.0.0.11"),
                (self.BASE_IP_PARAM, "10.0.1.11"),
                (self.HOST_PARAM, "***REMOVED***"),
                (self.PASSWORD_PATH_PARAM, "~/nova/src/other/secrets/bullet-password.txt"),
            ],
        )

        # Get interface
        self.declare_parameter(f"{self.device_prefix}.{self.INTERFACE_PARAM}", self.get_interface())
        self.interface = self.get_parameter(f"{self.device_prefix}.{self.INTERFACE_PARAM}").value

        # SSH commands
        self.ping_command = f"ping -c 3 -W 1 {self.get_parameter(f"{self.device_prefix}.{self.DEST_IP_PARAM}").value}"

        # Create SSH connection
        self.ssh_connection = Connection(
            host=f"{self.get_parameter(f"{self.device_prefix}.{self.HOST_PARAM}").value}@{self.get_parameter(f"{self.device_prefix}.{self.BASE_IP_PARAM}").value}",
            connect_kwargs={"password": open(os.path.expanduser(self.get_parameter(f"{self.device_prefix}.{self.PASSWORD_PATH_PARAM}").value)).read()},
            connect_timeout=3,
        )

        # Message Type, Topic Name, Quality of Service
        self.publisher = self.create_publisher(RadioStatus, "/chassis/radio_status", 10)

    def get_interface(self):
        '''
        Determines local network interface on base station 
        laptop used to reach base station radio
        Returns ethernet interface
        '''
        output = subprocess.check_output(f"ip route get {self.get_parameter(f"{self.device_prefix}.{self.BASE_IP_PARAM}").value}", shell=True, text=True)
        matches = re.findall(r"\bdev\s+(\S+)", output)
        if not matches:
            raise RuntimeError(f"Could not derive interface from route output: {output}")

        return matches[0]

    def connect_to_radio(self):
        '''
        Initiates ssh connection to the radio
        '''
        print(f"Connecting to {self.device} radio...")
        self.ssh_connection.open()
        print("Connected to radio!")

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
        # self.get_logger().info("Signal: %ddb, \tSent: %dkb, \tRecv: %dkb, \tPing: %dms" % (signal, sent, recv, ping))

        # Publish data over ROS
        radio_msg = RadioStatus()
        radio_msg.stamp = self.get_clock().now().to_msg()
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
            output = subprocess.check_output(self.ping_command, shell=True)
            matches = re.findall(rb" time=([\d.]+) ms", output)
            matches = [float(match) for match in matches]
            ms = sum(matches)/len(matches)

        # Returns this if there is a ping error
        except: 
            ms=9999
        
        return ms


# Main function sets up the ROS class
def main (args = None):
    # Run the monitor code
    rclpy.init(args = args)

    # Attempt to initialise the radio monitor
    try:
        radio_monitor = RadioMonitor()

        # Attempt to run the radio monitor code
        try:
            # Connect to the radio
            radio_monitor.connect_to_radio()

            # Forevery loop
            while True:
                radio_monitor.loop_function()
        
        # If issues found, raise exception
        except Exception as e:
            radio_monitor.get_logger().error(f"\nAn error occurred while running the radio monitor: {e}")

        # Shutdown cleanly
        radio_monitor.destroy_node()
        rclpy.shutdown()

    # If issues found, raise exception
    except Exception as e:
        print(f"\nAn error ocurred in initialising the radio monitor: {e}")

# Called when the script executes
if __name__=="__main__":
    main()
