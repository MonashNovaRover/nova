#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This file contains the ROS2 publisher code for 
    reading the spectrometer data. This includes
    date for both the BCA and FDA data.
This script also outputs the data to a CSV file
    stored in the data folder of science.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: spectrometer_publisher
TOPICS:
  - /science/spectrometer_data  [SpectrometerData]   [Published]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 
PACKAGE:     science 
AUTHOR(S):   Harrison Verrios
CREATION:    13/05/2022
EDITED:      15/05/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""  

from coms_utils.can_interface import CANReceiver

import rclpy, os, csv
from rclpy.node import Node

from pathlib import Path
from datetime import datetime

from core.msg import SpectrometerData 
    
# The ID for the CAN frame for BCA
BCA_CAN_ID = 0x008

# The number of BCA frames to receive
BCA_FRAMES = 3

# The ID for the CAN frame for FDA
FDA_CAN_ID = 0x007

# The number of FDA frames to receive
FDA_FRAMES = 5

# Whether or not to store the data in a csv
WRITE_TO_CSV = True

# The header row of the data
HEADER = ["cuvette", "scan", "valid", "bca(450nm)", "bca(500nm)", "bca(550nm)", "bca(570nm)", "bca(600nm)", "bca(650nm)",
    "fda(415nm)", "fda(445nm)", "fda(480nm)", "fda(515nm)", "fda(clear)", "fda(NIR)", "fda(555nm)", "fda(590nm)", 
    "fda(630nm)", "fda(680nm)", "fda(clear)", "fda(NIR)"]


# The spectrometer sensor publisher
class SpectrometerPublisher(Node):

    # Stores the current message value
    message: SpectrometerData = SpectrometerData()

    # Stores the current datetime for the file
    file_date: str = ""


    # Main constructor
    def __init__(self):

        # Initialises the node
        super().__init__('spectrometer_publisher')

        # Print initialisation information
        self.get_logger().warning("\033[92;1mInitialising the Spectrometer Publisher class.\033[0m")

        # Create the file name
        self.file_date = datetime.strftime(datetime.now(), "%Y-%m-%d_%H-%M-%S")

        # Reset the messsage
        self.new_message()
    
        # Create the CAN networks
        self.can_bca = CANReceiver(channel="can1", filter_ids=[BCA_CAN_ID], bitrate=500000)
        self.can_fda = CANReceiver(channel="can1", filter_ids=[FDA_CAN_ID], bitrate=500000)

        # Create the publisher over the network
        self.publisher = self.create_publisher(SpectrometerData, '/science/spectrometer_data', 10)
        
        # Create the timer to reads data from can
        self.timer = self.create_timer (0.01, self.read_data)



    # Unpacks CAN data and publishes it
    def read_data(self):

        # Cache for an errors that come about
        try:

            # Process the FDA
            self.process_data (self.can_fda, False, FDA_FRAMES)

            # Process the BCA
            self.process_data (self.can_bca, True, BCA_FRAMES)

        
        # In case of an eror, just continue
        except Exception as e:

            # Print the message
            self.get_logger().error("\033[91;1mERROR: %s\033[0m" % str(e))


        # Publish the message
        self.publish_message()


    # Process the data
    def process_data (self, can, is_bca: bool, frames: int):

        # Attempt to receive data from the BCA
        for frame in range(frames):
            can_msg = can.receive()

            # Get the current frame ID and if it is valid
            frame_id, valid, _, _ = can.unpack(can_msg.data, fmt=">bbhf")

            # Ensure the frame matches
            assert frame == frame_id

            # Ensure data is valid (and keep the valid)
            self.message.valid == (valid == 1) and self.message.valid

            # If invalid
            if not self.message.valid:
                raise Exception("Invalid data found. Skipping scan.")

               
            # If it is the header
            if frame == 0:
                _, _, cuvette, scan, _ = can.unpack(can_msg.data, fmt=">bbbbf")
                
                # Check if the cuvette is different
                if cuvette != self.message.cuvette:
                    self.new_message()

                # Check if the scan is different
                if scan != self.message.scan:
                    self.new_message()

                # Update the main stats
                self.message.cuvette = cuvette
                self.message.scan = scan

            # Otherwise, it is data
            else:
                _, _, d0, d1, d2 = can.unpack(can_msg.data, fmt=">bbHHH")

                # Go through each of the data
                for idx, d in enumerate([d0, d1, d2]):
                    index: int = idx + (frame - 1) * 3

                    # Store the data in the array
                    if is_bca:
                        self.message.bca[index] = d
                    else:
                        self.message.fda[index] = d                 


    # Handles the end of message
    def new_message (self):

        # Reset it
        self.message = SpectrometerData()

        # Create the arrays
        self.message.bca = [0] * 6
        self.message.fda = [0] * 12

        # Reset the valid to true
        self.message.valid = True


    # Handles publishing a message
    def publish_message (self):

        # Publish the message
        self.publisher.publish(self.message)

        # Print message
        self.get_logger().warning("\033[94;1mPublished Spectrometer Data for cuvette = %d, scan = %d\033[0m" % (self.message.cuvette, self.message.scan))

        # If writing to a CSV:
        if WRITE_TO_CSV:
        
            # Get the file of the csv
            directory: str = os.path.expanduser("~") + "/nova_ws/src/science/data/spectrometer/"
            filename: str = "%s_spectrometer.csv" % self.file_date
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
                row = [self.message.cuvette, self.message.scan, "VALID" if self.message.valid else "INVALID"]
                row.extend(self.message.bca)
                row.extend(self.message.fda)

                # Write the row
                writer.writerow(row)

            # Print the filename
            self.get_logger().warning("Spectrometer data successfully written to %s." % filepath)

        # Clear the message
        self.new_message()



# The main code that executes when starting
def main(args=None):

    # Create the publisher
    rclpy.init(args = args)
    publisher = SpectrometerPublisher()
    rclpy.spin(publisher)

    # Clean up when complete
    publisher.destroy_node()
    rclpy.shutdown()


# Called when the script executes
if __name__=="__main__":
    main()
