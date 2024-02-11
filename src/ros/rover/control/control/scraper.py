#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: A template ROS node
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: scraper
TOPICS:
  - subscriber: /control/input_joystick_l [InputJoystick]
  - subscriber: /control/input_joystick_r [InputJoystick]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:
AUTHOR(S):	Manika Goyal, Max Tory, Taaj Street
CREATION:	08/03/2023
EDITED:		18/03/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

"""
import rclpy
from rclpy.node import Node
import jcan, logging

# example of how to import a custom message type
from core.msg import InputJoystick

# an example of how to import a standard message type
from rclpy.qos import QoSReliabilityPolicy, QoSProfile
from rclpy.subscription import SubscriptionEventCallbacks
from rclpy.duration import Duration


class ScraperNode(Node):
    # set motor ids
    SCRAPER_ARM_ID_FORWARDS = 0x4
    SCRAPER_ARM_ID_BACKWARDS = 0x3
    SCRAPER_SCOOP_ID_FORWARDS = 0x6
    SCRAPER_SCOOP_ID_BACKWARDS = 0x5
    SCRAPER_BUCKET_ID_CLOSE = 0x8 # TODO: get correct value
    SCRAPER_BUCKET_ID_OPEN = 0x7 # TODO: get correct value
    

    def __init__(self):
        super().__init__("scraper")

        self.get_logger().set_level(logging.WARN)
        self.param_can = self.declare_parameter("can_bus", "can0").value
        self.param_scraper_arm_multiplier = self.declare_parameter("scraper_arm_multiplier", 200).value
        self.param_scraper_scoop_multiplier = self.declare_parameter("scraper_scoop_multiplier", 255).value
        self.param_scraper_bucket_default_velocity = self.declare_parameter("scraper_bucket_multiplier", 255).value # TODO: get correct value

        self.scraper_activated = False

        # Initially all motors spin backwards with 0 velocity
        self.scraper_arm_direction = self.SCRAPER_ARM_ID_BACKWARDS
        self.scraper_scoop_direction = self.SCRAPER_SCOOP_ID_BACKWARDS
        self.scraper_bucket_direction = self.SCRAPER_BUCKET_ID_OPEN
        self.scraper_arm_velocity = 0
        self.scraper_scoop_velocity = 0
        self.scraper_bucket_velocity = 0

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
        Sends can commands for scraper scoop, scraper arm together
        """
        # The list of values will be cast to uint8's by JCAN library - so be careful to double check the values!
        scraper_arm_commands, scraper_scoop_commands = self.get_scraper_can_commands()
        scraperArmFrame = jcan.Frame(0x0A0, scraper_arm_commands)
        scraperScoopFrame = jcan.Frame(0x0A0, scraper_scoop_commands)
        scraperBucketFrame = jcan.Frame(0x0A0, self.scraper_bucket_commands)

        self.get_logger().info(f"Sending {scraperArmFrame}")
        self.get_logger().info(f"Sending {scraperScoopFrame}")
        self.get_logger().info(f"Sending {scraperBucketFrame}")
        try:
            self.bus.send(scraperArmFrame)
            self.bus.send(scraperScoopFrame)
            # self.bus.send(scraperBucketFrame)

        except Exception as e:
            print(e)

    
    def deadline_callback(self, info):
        # Set all speeds to 0
        self.get_logger().warning("200ms Callback deadline missed")
        self.scraper_arm_velocity = 0
        self.scraper_arm_direction = self.SCRAPER_ARM_ID_FORWARDS
        self.scraper_scoop_velocity = 0
        self.scraper_scoop_direction = self.SCRAPER_SCOOP_ID_FORWARDS
        self.scraper_bucket_velocity = 0
        self.scraper_bucket_direction = self.SCRAPER_BUCKET_ID_OPEN


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
            self.joystick_lock = False


        if joystick_l.btn_bottom_l1_state >= 1 and self.scraper_activated:
            self.get_logger().info("Scraper OFF")
            self.scraper_activated = False

        # tile placer is unlocked when bottom r4 button is pressed on the right joystick
        # Tile Placer mode is on
        if joystick_l.btn_bottom_l4_state >= 1 and not self.scraper_activated:
            self.get_logger().info("Scraper ON")
            self.scraper_activated = True
            

        # Update the inputs
        if not self.joystick_lock and self.scraper_activated:
            self.scraper_arm_velocity = abs(int (self.param_scraper_arm_multiplier * joystick_l.ax_stick_x) )
            self.scraper_arm_direction = self.SCRAPER_ARM_ID_FORWARDS if joystick_l.ax_stick_x >= 0 else self.SCRAPER_ARM_ID_BACKWARDS
        
            #TODO: BUCKET OPEN AND CLOSE
            if joystick_l._btn_thumb_l_state >= 1:
                self.get_logger().info("Bucket OPEN")
                self.scraper_bucket_direction = self.SCRAPER_BUCKET_ID_OPEN
                self.scraper_bucket_velocity = self.param_scraper_bucket_default_velocity
            elif joystick_l._btn_thumb_r_state >= 1:
                self.get_logger().info("Bucket CLOSE")
                self.scraper_bucket_direction = self.SCRAPER_BUCKET_ID_CLOSE
                self.scraper_bucket_velocity = self.param_scraper_bucket_default_velocity
            else:
                self.scraper_bucket_direction = self.SCRAPER_BUCKET_ID_OPEN
                self.scraper_bucket_velocity = 0
        
        elif not self.joystick_lock and not self.scraper_activated:
            # joystick unlocked but scraper deactivated
            self.get_logger().info("Scraper OFF")
            self.scraper_arm_velocity = 0
            self.scraper_arm_direction = self.SCRAPER_ARM_ID_FORWARDS
        else:
            # if joysticks locked
            self.get_logger().info("joysticks locked!")
            self.scraper_arm_velocity = 0
            self.scraper_arm_direction = self.SCRAPER_ARM_ID_FORWARDS

    def joystick_r_callback(self, msg: InputJoystick):
        """
        Updates the classes internal msg state
        :param msg: core.msg.RoverPose message from the subscriber callback
        :return: None
        """
        self.get_logger().info("called r")

        joystick_r = msg

        if not self.joystick_lock and self.scraper_activated:
            # Update the inputs
            self.scraper_scoop_velocity = abs( int (self.param_scraper_scoop_multiplier * joystick_r.ax_stick_x) )
            self.scraper_scoop_direction = self.SCRAPER_SCOOP_ID_FORWARDS if joystick_r.ax_stick_x >= 0 else self.SCRAPER_SCOOP_ID_BACKWARDS
        elif not self.joystick_lock and not self.scraper_activated:
            # joystick unlocked but scraper deactivated
            self.get_logger().info("Scraper OFF")
            self.scraper_scoop_velocity = 0
            self.scraper_scoop_direction = self.SCRAPER_SCOOP_ID_FORWARDS
        else:
            # if joysticks locked
            self.get_logger().info("joysticks locked!")
            # set the scoop velocity to 0
            self.scraper_scoop_velocity = 0
            self.scraper_scoop_direction = self.SCRAPER_SCOOP_ID_FORWARDS

    def get_scraper_can_commands(self):
        scraper_arm_data = []
        scraper_arm_data.append(self.scraper_arm_direction)       
        scraper_arm_data.append(self.scraper_arm_velocity)

        scraper_scoop_data = []
        scraper_scoop_data.append(self.scraper_scoop_direction)        
        scraper_scoop_data.append(self.scraper_scoop_velocity)

        return scraper_arm_data, scraper_scoop_data


def main():
    rclpy.init()
    scraper = ScraperNode()
    rclpy.spin(scraper)
    rclpy.shutdown()


if __name__ == "__main__":
    main()
