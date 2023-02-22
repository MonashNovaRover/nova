import serial

import rclpy
from rclpy.node import Node

from core.msg import RoverPoseGPS
from rclpy.logging import LoggingSeverity

class SkytraqNode (Node):
    def __init__ (self, com_no, baud):
        super().__init__('gps_data')

        self.pose = RoverPoseGPS()

        self.pose.latitude, self.pose.longitude = 0, 0
        self.pose.pitch, self.pose.roll, self.pose.yaw  = 0, 0, 0
        self.pose.valid = False

        self.ser = serial.Serial()
        self.config_port(com_no, baud)

        self.publisher = self.create_publisher(RoverPoseGPS, 'gps_data', 10)
        self.timer = self.create_timer(0.5, self.publisher_callback)

    def parse_msg(self, pose):
        raw_msg = self.get_msg()

        if raw_msg[0:2] == ["b'$PSTI", '036']:
            pose.pitch, pose.roll, pose.yaw = raw_msg[4], raw_msg[5], raw_msg[6]
        # elif raw_msg[0:2] == ['PSTI', '032']:
        #     db
        elif raw_msg[0] == "b'$GNRMC":
            if raw_msg[2] == 'A':
                pose.lat, pose.lon = raw_msg[3], raw_msg[5]
                pose.valid = True
    
    def get_msg(self):
        txt = str(self.ser.readline())
        return txt.split(",")

    def print_msg(self, rover_msg):
        roverMsgStr = f"""
        valid: {rover_msg.valid:8.3f}
        lat: {rover_msg.latitude:8.3f}
        lon: {rover_msg.longitude:8.3f}
        pitch: {rover_msg.pitch:8.2f}
        roll: {rover_msg.roll:8.2f}
        yaw: {rover_msg.yaw:8.2f}
        """

        if rover_msg.valid:
            self.get_logger().log(roverMsgStr,LoggingSeverity.INFO,throttle_duration_sec=2)
        else:
            self.get_logger().log(roverMsgStr,LoggingSeverity.WARN,throttle_duration_sec=2)

    def publisher_callback(self):
        self.parse_msg(self.pose)
        self.publisher.publish(self.pose)





    
        
def main (args = None):
    rclpy.init(args = args)
    gps = SkytraqNode()
    rclpy.spin(gps)
    
    gps.destroy_node()
    rclpy.shutdown()
    
if __name__ == "__main__":
    main()