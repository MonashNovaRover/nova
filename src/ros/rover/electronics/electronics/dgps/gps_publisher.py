import serial

import rclpy
from rclpy.node import Node

from core.msg import RoverPoseGPS
from rclpy.logging import LoggingSeverity

class SkytraqNode (Node):
    def __init__ (self, com_no, baud):
        super().__init__('gps_data')

        self.pose = RoverPoseGPS()

        self.fix_type = "NA"

        self.pose.latitude, self.pose.longitude = 0.0, 0.0
        self.pose.pitch, self.pose.roll, self.pose.yaw  = 0.0, 0.0, 0.0
        self.pose.valid = False

        self.ser = serial.Serial()
        self.config_port(com_no, baud)

        self.counter = 0

        self.publisher = self.create_publisher(RoverPoseGPS, 'gps_data', 10)
        self.timer = self.create_timer(0.1, self.publisher_callback)

    def parse_msg(self, pose):
        raw_msg = self.get_msg()

        if raw_msg[0:2] == ["b'$PSTI", '036']:
            if raw_msg[4] != '' and (raw_msg[4].isupper() or raw_msg[4].islower()) == False:
                pose.pitch, pose.roll, pose.yaw = float(raw_msg[5]), float(raw_msg[6]), float(raw_msg[4])
        # elif raw_msg[0:2] == ['PSTI', '032']:
        #     db
        elif raw_msg[0] == "b'$GNRMC":
            pose.latitude, pose.longitude = float(raw_msg[3]), float(raw_msg[5])
            if raw_msg[2] == 'A':
                pose.valid = True
            else:
                pose.valid = False
            if raw_msg[10] != "":
                self.fix_type = raw_msg[10]

            debug_str = f"""{raw_msg[6]}, {raw_msg[7]}, {raw_msg[8]}, {raw_msg[9]}, {raw_msg[10]}, {raw_msg[11]}, {raw_msg[12]}"""
            self.get_logger().log(debug_str,LoggingSeverity.INFO,throttle_duration_sec=2)
            
    
    def get_msg(self):
        self.counter+=1
        if self.counter > 30:
            self.ser.reset_input_buffer()
            self.counter = 0

        txt = str(self.ser.readline())
        return txt.split(",")
    
    def config_port(self, port_name, baud):
        self.ser.baudrate = baud
        if port_name == "":
            port_name = "/dev/ttyUSB0"
        self.ser.port = port_name
        self.ser.open()

    def print_msg(self, rover_msg):
        roverMsgStr = f"""
        valid: {rover_msg.valid}
        fix type: {self.fix_type}
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
        self.print_msg(self.pose)





    
        
def main (args = None):
    baud = 115200;
    rclpy.init(args = args)
    gps = SkytraqNode("", baud)
    rclpy.spin(gps)
    
    gps.destroy_node()
    rclpy.shutdown()
    
if __name__ == "__main__":
    main()

# Serial.reset_input_buffer