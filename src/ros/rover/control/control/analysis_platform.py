#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Control Analysis Arm Platform using Joysticks
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: analysis_platform
TOPICS:
  - subscriber: /control/input_joystick_l [InputJoystick]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    control
AUTHOR(S):	Tristan Clark
CREATION:	08/03/2024
EDITED:		08/03/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

"""
from control.control_classes import Direction, JonoCardController, OneAxisControlLimits
import rclpy, jcan, logging
from rclpy.node import Node
from rclpy.qos import QoSReliabilityPolicy, QoSProfile
from rclpy.subscription import SubscriptionEventCallbacks
from rclpy.duration import Duration
from sensor_msgs.msg import Range

# import the joystick ROS message we are listening to
from core.msg import InputJoystick


class AnalysisPlatformNode(Node):
    # can bus
    CAN_BUS = "can1"
    # card IDs
    JONO_ID_PLATFORM = 0x0A0
    JONO_ID_HYDRAPROBE = 0x0A0
    JONO_ID_TIME_OF_FLIGHT = 0x4A1
    JONO_ID_LIMIT_SWITCH = 0x4A2
    # command directions
    PLATFORM_UP_COMMAND = 0x01
    PLATFORM_DOWN_COMMAND = 0x02
    # directions
    PLATFORM_UP = Direction.POSITIVE
    PLATFORM_DOWN = Direction.NEGATIVE
    # max_velocity
    MAX_VELOCITY_PERCENT = 0.75
    # time of flight
    TIME_OF_FLIGHT_OFFSET = 10
    TIME_OF_FLIGHT_BOTTOM = 20
    # limit switch
    PLATFORM_LIMIT_SWITCH_TOP = 0x03
    LIMIT_SWITCH_CLEAR = 0x00
    LIMIT_SWITCH_HIT = 0x01
    # ROS parameter names
    CAN_BUS_PARAM = "can_bus"
    PLATFORM_MAX_VEL_PERCENT_PARAM = "platform_max_vel_percent"
    TIME_OF_FLIGHT_BOTTOM_PARAM = "time_of_flight_bottom"

    def __init__(self):
        super().__init__("analysis_platform")

        self.get_logger().set_level(logging.DEBUG)

        # ROS parameters: used so that they can be changed without recompiling or rerunning the node
        self.declare_parameter(self.CAN_BUS_PARAM, self.CAN_BUS)
        self.declare_parameter(self.PLATFORM_MAX_VEL_PERCENT_PARAM, self.MAX_VELOCITY_PERCENT)
        self.declare_parameter(self.TIME_OF_FLIGHT_BOTTOM_PARAM, self.TIME_OF_FLIGHT_BOTTOM)

        # Create the custom control classes
        # Platform control
        self.platform = OneAxisControlLimits(
            max_percent=self.get_parameter(self.PLATFORM_MAX_VEL_PERCENT_PARAM).value
        )
        # Platform card controller for Jono Card
        self.platform_controller = JonoCardController(
            card_id=self.JONO_ID_PLATFORM, 
            pos_command=self.PLATFORM_UP_COMMAND, 
            neg_command=self.PLATFORM_DOWN_COMMAND, 
            control=self.platform
        )

        self.joystick_lock = True

        # Set up the QoS profile and deadline callback
        deadline = Duration(nanoseconds=2e8)        
        events = SubscriptionEventCallbacks(deadline=self.deadline_callback)
        self.qos = QoSProfile(reliability=QoSReliabilityPolicy.BEST_EFFORT, depth=1, deadline=deadline)

        # Create the joystick subscribers
        self.joystick_l_sub = self.create_subscription(InputJoystick, "/control/input_joystick_l", self.joystick_l_callback, self.qos, event_callbacks=events)

        # Create the time of flight publisher
        self.publisher = self.create_publisher(Range, "/control/analysis_platform", 10)

        self.bus = jcan.Bus()
        self.bus.set_id_filter([self.JONO_ID_TIME_OF_FLIGHT, self.JONO_ID_LIMIT_SWITCH])

        # Add callbacks for receiving can feedback
        # Time of flight sensor and limit switch
        self.bus.add_callback(self.JONO_ID_TIME_OF_FLIGHT, self.callback_receive_can_time_of_flight)
        self.bus.add_callback(self.JONO_ID_LIMIT_SWITCH, self.callback_receive_can_limit_switch)

        # Open the can bus
        self.bus.open(self.get_parameter(self.CAN_BUS_PARAM).value)

        # Create timers for sending can commands and spinning the bus
        self.timer_jcan_commands = self.create_timer(0.05, self.callback_send_can_commands)
        self.timer_jcan_spin = self.create_timer(0.01, self.bus.spin)


    def callback_send_can_commands(self):
        """
        Take current internal state and publish over CAN
        Sends can commands for auger and drill together
        """
        # The list of values will be cast to uint8's by JCAN library - so be careful to double check the values!
        platformFrame = self.platform_controller.get_frame()

        self.get_logger().info(f"Sending {platformFrame}")

        try:
            self.bus.send(platformFrame)

        except Exception as e:
            print(e)

    def convert_time_of_flight(self, raw_height: int) -> float:
        """
        Convert raw time of flight sensor reading (in cm)
        to remove offset height and a base bottom reading greater than 0
        Final value should be close to real height (in cm)
        """
        height = raw_height - self.TIME_OF_FLIGHT_OFFSET - self.get_parameter(self.TIME_OF_FLIGHT_BOTTOM_PARAM).value
        if height < 0:
            height = 0
        return height
    

    def callback_receive_can_time_of_flight(self, frame: jcan.Frame):
        """Receive can feedback for auger limit switches
        """
        self.get_logger().info(f"Received {hex(frame.id)} {frame.data}")
 
        if frame.id == self.JONO_ID_TIME_OF_FLIGHT:
            if len(frame.data) != 2:
                self.get_logger().info(f"Time of flight error")
                return
            raw_height = int(frame.data[1] + (frame.data[0] << 8))
            height = self.convert_time_of_flight(raw_height)
            self.get_logger().info(f"Raw height: {raw_height}, Converted height: {height}")
            if height == 0:
                self.get_logger().info("Time of flight hit bottom")
                self.platform.update_limit_neg(True)
            else:
                self.platform.update_limit_neg(False)

            self.publish_time_of_flight(height)
        else:
            self.get_logger().info(f"Received unknown frame {frame}")


    def callback_receive_can_limit_switch(self, frame: jcan.Frame):
        """Receive can feedback for auger limit switches
        """
        self.get_logger().info(f"Received {hex(frame.id)} {frame.data}")
 
        if frame.id == self.JONO_ID_LIMIT_SWITCH:
            if frame.data[0] == self.PLATFORM_LIMIT_SWITCH_TOP:
                if frame.data[1] == self.LIMIT_SWITCH_HIT:
                    self.get_logger().info("Top limit switch hit")
                    self.platform.update_limit_pos(True)
                else:
                    self.platform.update_limit_pos(False)
        else:
            self.get_logger().info(f"Received unknown frame {frame}")
        
    def deadline_callback(self, _info):
        """
        Callback for when the deadline is missed
        """
        # Set all speeds to 0
        self.get_logger().warning("200ms Callback deadline missed")
        self.platform.stop()

    def check_joystick_lock(self):
        """
        Checks if the joysticks are locked
        """
        if self.joystick_lock:
            self.get_logger().info("Joysticks locked!")
            self.platform.stop()
            return True
        return False

    def update_joystick_lock(self, joystick_l: InputJoystick):
        # Joysticks lock if botton L2 button is pressed on the left joystick
        if joystick_l.btn_bottom_l2_state >= 1 and not self.joystick_lock:
            self.get_logger().info("Joysticks Locked")
            self.joystick_lock = True

        # Joysticks unlocked when bottom l5 button is pressed on the left joystick
        if joystick_l.btn_bottom_l5_state >= 1 and self.joystick_lock:
            self.get_logger().info("Joysticks Unlocked")
            self.joystick_lock = False

    def update_platform_height(self, joystick_l: InputJoystick):
        # analysis platform height direction is determined by the right joystick's x-axis direction
        self.platform.update_direction(self.PLATFORM_DOWN if joystick_l.ax_stick_x >= 0 else self.PLATFORM_UP)
        self.platform.update_velocity(abs(int(joystick_l.ax_stick_x)))

        # button override time of flight
        # allows operators to lower the platform even 
        # if the time of flight sensor is reading the 0 / reached bottom
        if joystick_l.btn_thumb_d_state >= 1:
            self.platform.update_velocity(velocity=0.1, ignore_limits=True)
            self.platform.update_direction(self.PLATFORM_DOWN)
        
       
    def joystick_l_callback(self, msg: InputJoystick):
        """
        Updates the classes internal msg state
        :return: None
        """
        self.get_logger().info("called l")

        joystick_l = msg

        self.update_joystick_lock(joystick_l)

        if self.check_joystick_lock():
            return
        
        self.update_platform_height(joystick_l)

    def publish_time_of_flight(self, height: int):
        """
        Publishes the height of the platform
        """
        msg = Range()
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.header.frame_id = "analysis_platform"
        msg.min_range = 0.0
        msg.max_range = 100.0
        msg.range = float(height)
        self.publisher.publish(msg)

def main():
    rclpy.init()
    node = AnalysisPlatformNode()
    rclpy.spin(node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()
