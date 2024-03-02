#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Control Tile Placer using Joysticks
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: tile_placer
TOPICS:
  - subscriber: /control/input_joystick_l [InputJoystick]
  - subscriber: /control/input_joystick_r [InputJoystick]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: control
COMMAND: ros2 run control tile_placer.py
RUN ON: Rover
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
AUTHOR(S):	Tristan Clark
CREATION:	02/02/2024
EDITED:		02/02/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

"""
import rclpy, jcan, logging
from struct import pack
from rclpy.node import Node
from rclpy.qos import QoSReliabilityPolicy, QoSProfile
from rclpy.subscription import SubscriptionEventCallbacks
from rclpy.duration import Duration

# import the joystick ROS message we are listening to
from core.msg import InputJoystick

class TilePlacerNode(Node):
    # can bus
    CAN_BUS = "can0"
    # card IDs
    TILE_PLACER_ID = 0x063
    SCRAPER_SCOOP_ID = 0x053
    # command data
    SCRAPER_ARM_FORWARDS = 1
    SCRAPER_ARM_BACKWARDS = -1
    SCRAPER_SCOOP_FORWARDS = 1
    SCRAPER_SCOOP_BACKWARDS = -1
    # max_velocity
    MAX_VELOCITY = 32767 * (4/5) # 4/5 of max possible value sent to motor

    # TODO: scraper bucket does not exist yet
    # check these when implemented
    SCRAPER_BUCKET_ID = 0x073 
    SCRAPER_BUCKET_OPEN = 1
    SCRAPER_BUCKET_CLOSE = -1
    # ROS param names
    CAN_BUS_PARAM = "can_bus"
    SCRAPER_ARM_MAX_VEL_PARAM = "scraper_arm_max_vel"
    SCRAPER_SCOOP_MAX_VEL_PARAM = "scraper_scoop_max_vel"
    SCRAPER_BUCKET_MAX_VEL_PARAM = "scraper_bucket_max_vel"
    
    # set motor ids
    TILE_PLACER_ID_UP = 0x2
    TILE_PLACER_ID_DOWN = 0x1

    def __init__(self):
        super().__init__("tile_placer")

        self.get_logger().set_level(logging.DEBUG)
        self.param_can = self.declare_parameter("can_bus", "can0").value
        self.param_tile_placer_default_velocity = self.declare_parameter("tile_placer_default_velocity", 200).value

        self.tile_placer_activated = False

        # Initially all motors spin backwards with 0 velocity
        self.tile_placer_direction = self.TILE_PLACER_ID_DOWN
        self.tile_placer_velocity = 0

        self.joystick_lock = True


        deadline = Duration(nanoseconds=2e8)        
        events = SubscriptionEventCallbacks(deadline=self.deadline_callback)
        self.qos = QoSProfile(reliability=QoSReliabilityPolicy.BEST_EFFORT, depth=1, deadline=deadline)

        self.joystick_l_sub = self.create_subscription(InputJoystick, "/control/input_joystick_l", self.joystick_l_callback, self.qos, event_callbacks=events)
        self.joystick_r_sub = self.create_subscription(InputJoystick, "/control/input_joystick_r", self.joystick_r_callback, self.qos, event_callbacks=events)

        self.bus = jcan.Bus()
        self.bus.open(self.param_can)
        self.timer_jcan = self.create_timer(0.05, self.callback_send_can_commands)


    def callback_send_can_commands(self):
        """Take current internal state and publish over CAN
        Sends can commands for tile placer
        """
        # The list of values will be cast to uint8's by JCAN library - so be careful to double check the values!
        tile_placer_commands = self.get_tile_placer_can_commands()
        tilePlacerFrame = jcan.Frame(0x0A0, tile_placer_commands)

        self.get_logger().info(f"Sending {tilePlacerFrame}")
        try:
            self.bus.send(tilePlacerFrame)

        except Exception as e:
            print(e)

    
    def deadline_callback(self, info: InputJoystick):
        # Set all speeds to 0
        self.get_logger().warning("200ms Callback deadline missed")
        self.tile_placer_stop_state()

    def tile_placer_stop_state(self):
        self.tile_placer_velocity = 0
        self.tile_placer_direction = self.TILE_PLACER_ID_UP


    def joystick_l_callback(self, msg: InputJoystick):
        """
        Updates the classes internal msg state
        :return: None
        """
        self.get_logger().info("called l")

        joystick_l = msg

        # Joysticks lock if botton L2 button is pressed on the left joystick
        if joystick_l.btn_bottom_l2_state >= 1 and not self.joystick_lock:
            self.get_logger().info("Joysticks Locked")
            self.joystick_lock = True

        # joysticks are only unlocked when bottom l5 button is pressed on the left joystick
        if joystick_l.btn_bottom_l5_state >= 1 and self.joystick_lock:
            self.get_logger().info("Joysticks Unlocked")
            if self.tile_placer_activated:
                self.get_logger().info("Tile Placer ON")
            else:
                self.get_logger().info("Tile Placer OFF")
            self.joystick_lock = False


    def joystick_r_callback(self, msg: InputJoystick):
        """
        Updates the classes internal msg state
        :param msg: core.msg.RoverPose message from the subscriber callback
        :return: None
        """
        self.get_logger().info("called r")

        joystick_r = msg

        # Tile Placer mode is off
        if joystick_r.btn_bottom_r1_state >= 1 and self.tile_placer_activated:
            self.get_logger().info("Tile Placer OFF")
            self.tile_placer_activated = False

        # tile placer is unlocked when bottom r4 button is pressed on the right joystick
        # Tile Placer mode is on
        if joystick_r.btn_bottom_r4_state >= 1 and not self.tile_placer_activated:
            self.get_logger().info("Tile Placer ON")
            self.tile_placer_activated = True

        if not self.joystick_lock and self.tile_placer_activated:
            self.get_logger().info("Tile Placer ON")
            # Update the inputs
            if joystick_r._btn_thumb_l_state >= 1:
                self.get_logger().info("Tile Placer UP")
                self.tile_placer_direction = self.TILE_PLACER_ID_UP
                self.scraper_bucket_velocity = self.param_tile_placer_default_velocity
            elif joystick_r._btn_thumb_r_state >= 1:
                self.get_logger().info("Tile Placer DOWN")
                self.scraper_bucket_direction = self.TILE_PLACER_ID_DOWN
                self.scraper_bucket_velocity = self.param_tile_placer_default_velocity
            else:
                self.tile_placer_stop_state()
         

        elif not self.joystick_lock and not self.tile_placer_activated:
            self.get_logger().info("Tile Placer OFF")
            self.tile_placer_stop_state()
        else:
            # if joysticks locked
            self.get_logger().info("Joysticks locked!")
            self.tile_placer_activated = False
            self.tile_placer_stop_state()


    def get_tile_placer_can_commands(self):
        tile_placer_data = []
        tile_placer_data.append(self.tile_placer_direction)        
        tile_placer_data.append(self.tile_placer_velocity)

        return tile_placer_data


def main():
    rclpy.init()
    tile_placer = TilePlacerNode()
    rclpy.spin(tile_placer)
    rclpy.shutdown()


if __name__ == "__main__":
    main()
