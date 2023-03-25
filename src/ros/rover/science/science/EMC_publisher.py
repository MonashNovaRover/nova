#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This file contains the ROS2 receiver code for the EMC
    publisher, which reads data from CAN and pushes
    the data over ROS.
This includes data for the Temperature, Pressure,
    Humidity and Wind Speed.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: emc_publisher
TOPICS:
  - /science/emc_data  [EMCData]   [Published]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	 science
AUTHOR(S):	 Kelly Huang, Harrison Verrios
CREATION:	 05/05/2022
EDITED:		 15/05/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

from coms_utils.can_interface import CANReceiver

import rclpy, os, csv
from rclpy.node import Node

from pathlib import Path
from datetime import datetime

from core.msg import EMCData 
    
# The ID for the CAN frame for the EMC sensor module
EMC_CAN_ID = 0x009

# The number of EMC frames to receive
EMC_FRAMES = 3

# The ID for the Wind Sensor
WIND_CAN_ID = 0x00A

# Whether or not to store the data in a csv
WRITE_TO_CSV = True

# The header row of the data
HEADER = ["timestamp", "temperature(C)", "pressure(Pa)", "humidity(%)", "wind speed(m/s)"]


# The EMC sensor publisher
class EMCPublisher(Node):

    # Stores the current message value
    message: EMCData = EMCData()

    # Stores the current datetime for the file
    file_date: str = ""


    # Main constructor
    def __init__(self):

        super().__init__('emc_publisher')

        # Print initialisation information
        self.get_logger().warning("\033[92;1mInitialising the EMC Publisher class.\033[0m")

        # Create the file name
        self.file_date = datetime.strftime(datetime.now(), "%Y-%m-%d_%H-%M-%S")

        # Reset the messsage
        self.new_message()
    
        # Create the CAN networks
        self.can_emc =  CANReceiver(channel="can1", filter_ids=[EMC_CAN_ID], bitrate=500000)
        self.can_wind = CANReceiver(channel="can1", filter_ids=[WIND_CAN_ID], bitrate=500000)

        # Create the publisher over the network
        self.publisher = self.create_publisher(EMCData, '/science/emc_data', 10)
        
        # Create the timer to reads data from can
        self.timer = self.create_timer (0.01, self.read_data)


    # Unpacks CAN data and publishes it
    def read_data(self):

        # Cache for an errors that are created
        try:

            # Process the EMC data on the nework
            self.process_emc()

            # Process the Wind sensor data on the network
            self.process_wind ()

        
        # In case of an eror, just continue
        except Exception as e:

            # Print the message
            self.get_logger().error("\033[91;1mERROR: %s\033[0m" % str(e))


        # Publish the message
        self.publish_message()


    # Process the EMC data for temperature, pressure and humidity
    def process_emc (self):

        # Attempt to receive data from the BCA
        for frame in range(EMC_FRAMES):
            can_msg = self.can_emc.receive()

            # Get the current frame id and the data
            frame_id, data = self.can_emc.unpack(can_msg.data, fmt="<bf")

            # Ensure the frame matches
            assert frame == frame_id

            # Store the data
            if frame == 0:
                self.message.temperature = data
            elif frame == 1:
                self.message.pressure = data
            elif frame == 2:
                self.message.humidity = data
            else:
                raise Exception("Invalid frame: %d." % frame)

    
    # Process the wind sensor data
    def process_wind (self):

        # Get the data and unpack it to a float
        can_msg = self.can_wind.receive()
        data = self.can_wind.unpack(can_msg.data, fmt="<f")

        # Store the data in the message
        self.message.wind = data[0]


    # Handles the end of message
    def new_message (self):

        # Reset it
        self.message = EMCData()


    # Handles publishing a message
    def publish_message (self):

        # Publish the message
        self.publisher.publish(self.message)

        # Print message
        self.get_logger().warning("\033[94;1mPublished EMC Data.\033[0m")

        # If writing to a CSV:
        if WRITE_TO_CSV:
        
            # Get the file of the csv
            directory: str = os.path.expanduser("~") + "/nova_ws/src/rover/science/data/emc/"
            filename: str = "%s_emc.csv" % self.file_date
            filepath: str = "%s/%s" % (directory, filename)

            # Make sure the directory is valid
            Path(directory).mkdir(parents=True, exist_ok=True)

            # If the path does not exist, create the header
            make_header = not os.path.exists(filepath)

            # Open up the file
            with open(filepath, "a", newline='') as csvfile:
                writer = csv.writer(csvfile, delimiter=',')

                # If making the header
                if make_header:
                    writer.writerow(HEADER)

                # Create the row
                row = [datetime.now(), self.message.temperature, self.message.pressure, self.message.humidity, self.message.wind]

                # Write the row
                writer.writerow(row)

            # Print the filename
            self.get_logger().warning("EMC Data successfully written to %s." % filepath)

        # Clear the message
        self.new_message()


# The main code that executes when starting
def main(args=None):

    # Create the publisher
    rclpy.init(args = args)
    publisher = EMCPublisher()
    rclpy.spin(publisher)

    # Clean up when complete
    publisher.destroy_node()
    rclpy.shutdown()


# Called when the script executes
if __name__=="__main__":
    main()
