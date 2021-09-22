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
  - /electronics/radio_status [RadioStatus]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	electronics
AUTHOR(S):	Emily Kuo
CREATION:	22/09/2021
EDITED:		22/09/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
  -allow for non-ros
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import rclpy
from rclpy.node import Node
from core.msg import RadioStatus
from fabric import Connection
import re
import time
import psutil
import subprocess
import sys

from rclpy.timer import Rate

dev_info = {
    "METABOX": {
        "is_ros": 1,                  # ROS = 1, else standard 0
        "dest_IP": "192.168.1.204",   # Destination IP of Jetson
        "radio_IP": "192.168.3.155",  # Base station radio IP
        "password": "rovanova",       # Password of Radio device
        "interface": "enp3s0f1",      # Ethernet Interface 
    },
    "MAC": {
        "is_ros": 0,                  # ROS = 1, else standard 0
        "dest_IP": "192.168.1.204",   # Destination IP of Jetson
        "radio_IP": "192.168.3.155",  # Base station radio IP
        "password": "rovanova",       # Password of Radio device
        "interface": "enp1s0f0",      # Ethernet Interface 
    },
}

class RadioMonitor(Node):
  def __init__(self, device):
    print("Initialising Radio Monitor class")

    self.is_ros = dev_info[device]["is_ros"]
    self.dest_IP = dev_info[device]["dest_IP"]
    self.radio_IP = dev_info[device]["radio_IP"]
    self.password = dev_info[device]["password"]
    self.interface = dev_info[device]["interface"]

    self.command = "ping -c 3 -W 1 " + self.dest_IP #shell command
    self.ssh_connection = Connection(host="nova@"+self.radio_IP, connect_kwargs={"password":self.password}, connect_timeout=3)

    if self.is_ros:
      super().__init__("radio_monitor")
      self.radio_publisher = self.create_publisher(RadioStatus, "/electronics/radio_status", 10)

    def connect_to_radio(self):
      '''
      Initiates ssh connection to the radio
      '''
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

      print("Signal strength: %0.0fdb" % signal)
      print("Sent: %0.0f kbits" % sent)
      print("Received: %0.0f kbits" % recv)
      print("Pinging %s, Time elapsed: %0.0fms \n" % (self.dest_IP, ping))

      if self.is_ros:
        radio_msg = RadioStatus()
        radio_msg.signal = signal
        radio_msg.sent = sent
        radio_msg.recv = recv
        radio_msg.ping = ping
        self.radio_publisher.publish(radio_msg)
    
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

    def get_ping(self):
      '''
      Pings destination IP three times and averages the result
      Returns ping (ms)
      '''
      try: 
        output = subprocess.check_output(self.command, shell=True)
        matches = re.findall(" time=([\d.]+) ms", output)
        matches = [float(match) for match in matches]
        ms = sum(matches)/len(matches)
      except: 
        ms=9999 #returns this if there is a ping error
      
      return ms

def main():
  try:
    radio_monitor.connect_to_radio()
    while True:
        radio_monitor.loop_function()
  
  except Exception as e:
    print(e)

if __name__=="__main__":
    # Grab parameter and check if not existed
  if len(sys.argv) <= 1:
      # Output error message
      print("Please enter a configuration out of the following:")
      for dev in dev_info.keys():
          print("\t" + dev)
      print("Using Metabox as the default settings.")
      device = "Metabox"

  # Otherwise take the parameter
  else:
      # If found an invalid parameter
      if sys.argv[1].upper() not in dev_info.keys():
          print("Using Metabox as the default settings.")
          device = "Metabox"
      # Else set the device
      else:
          device = sys.argv[1].upper()

  # Run the monitor code
  rclpy.init()
  radio_monitor = RadioMonitor(device)
  main()