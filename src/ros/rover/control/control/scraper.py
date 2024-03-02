#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Control Scraper using Joysticks
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: scraper
TOPICS:
  - subscriber: /control/input_joystick_l [InputJoystick]
  - subscriber: /control/input_joystick_r [InputJoystick]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: control
COMMAND: ros2 run control scraper.py
RUN ON: Rover
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
AUTHOR(S):	Tristan Clark
CREATION:	02/02/2024
EDITED:		02/02/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

"""
import rclpy, jcan, logging
from enum import Enum
from struct import pack
from rclpy.node import Node
from rclpy.qos import QoSReliabilityPolicy, QoSProfile
from rclpy.subscription import SubscriptionEventCallbacks
from rclpy.duration import Duration

# import the joystick ROS message we are listening to
from core.msg import InputJoystick

class Controller(Enum):
    CMD = "CMD"
    JONO_CARD = "JONO_CARD"

class Direction(Enum):
    ONE = 1
    TWO = -1

CARD = Controller.CMD

class ScraperNode(Node):
    # can bus
    CAN_BUS = "can0"

    # card IDs
    # only used when CARD = CMD
    CMD_SCRAPER_ARM_ID = 0x063
    CMD_SCRAPER_SCOOP_ID = 0x053
    # only used when CARD = JONO_CARD
    JONO_CARD_ID = 0x0A0

    # command data
    SCRAPER_ARM_FORWARDS = Direction.ONE
    SCRAPER_ARM_BACKWARDS = Direction.TWO
    SCRAPER_SCOOP_FORWARDS = Direction.ONE
    SCRAPER_SCOOP_BACKWARDS = Direction.TWO

    # max_velocity
    CMD_MAX_VELOCITY = int(32767 * (4/5)) # 4/5 of max possible value sent to motor
    JONO_CARD_MAX_VELOCITY = 255
    
    # ROS param names
    CAN_BUS_PARAM = "can_bus"
    CARD_TYPE_PARAM = "card_type"
    SCRAPER_ARM_MAX_VEL_PARAM = "scraper_arm_max_vel"
    SCRAPER_SCOOP_MAX_VEL_PARAM = "scraper_scoop_max_vel"
    SCRAPER_BUCKET_MAX_VEL_PARAM = "scraper_bucket_max_vel"


    # TODO: scraper bucket does not exist yet
    # check these when implemented
    SCRAPER_BUCKET_CMD_ID = 0x073 
    SCRAPER_BUCKET_OPEN = Direction.ONE
    SCRAPER_BUCKET_CLOSE = Direction.TWO

    

    def __init__(self):
        super().__init__("scraper")

        self.get_logger().set_level(logging.DEBUG)
        self.get_logger().info("Initialising Scraper Node...")

        # Setting ROS parameters
        # This is done so that the parameters can be changed during runtime if desired
        self.declare_parameter(self.CAN_BUS_PARAM, self.CAN_BUS)
        self.declare_parameter(self.CARD_TYPE_PARAM, CARD.value)

        max_vel = self.CMD_MAX_VELOCITY if (self.get_parameter(self.CARD_TYPE_PARAM).value == Controller.CMD) else self.JONO_CARD_MAX_VELOCITY
        self.declare_parameter(self.SCRAPER_ARM_MAX_VEL_PARAM, max_vel)
        self.declare_parameter(self.SCRAPER_SCOOP_MAX_VEL_PARAM, max_vel)
        self.declare_parameter(self.SCRAPER_BUCKET_MAX_VEL_PARAM, max_vel)

        # Initially all motors spin backwards with 0 velocity
        self.scraper_arm_direction = self.SCRAPER_ARM_BACKWARDS
        self.scraper_scoop_direction = self.SCRAPER_SCOOP_BACKWARDS
        self.scraper_bucket_direction = self.SCRAPER_BUCKET_OPEN
        self.scraper_arm_velocity = 0
        self.scraper_scoop_velocity = 0
        self.scraper_bucket_velocity = 0

        self.scraper_lock = True
        self.joystick_lock = True


        deadline = Duration(nanoseconds=2e8)        
        events = SubscriptionEventCallbacks(deadline=self.deadline_callback)
        self.qos = QoSProfile(reliability=QoSReliabilityPolicy.BEST_EFFORT, depth=1, deadline=deadline)

        self.joystick_l_sub = self.create_subscription(InputJoystick, "/control/input_joystick_l", self.joystick_l_callback, self.qos, event_callbacks=events)
        self.joystick_r_sub = self.create_subscription(InputJoystick, "/control/input_joystick_r", self.joystick_r_callback, self.qos, event_callbacks=events)

        self.bus = jcan.Bus()
        self.bus.open(self.get_parameter(self.CAN_BUS_PARAM).value)
        self.timer_jcan = self.create_timer(0.05, self.callback_send_can_commands)

        self.get_logger().info("Scraper Node Initialised")


    def callback_send_can_commands(self):
        """
        Take current internal state and publish over CAN
        Sends can commands for scraper scoop, scraper arm together
        """
        # The list of values will be cast to uint8's by JCAN library - so be careful to double check the values!
        scraper_arm_commands, scraper_scoop_commands, scraper_bucket_commands = self.get_can_commands()
        scraperArmFrame = jcan.Frame(self.CMD_SCRAPER_ARM_ID, scraper_arm_commands)
        scraperScoopFrame = jcan.Frame(self.CMD_SCRAPER_SCOOP_ID, scraper_scoop_commands)
        scraperBucketFrame = jcan.Frame(self.SCRAPER_BUCKET_CMD_ID, scraper_bucket_commands)

        self.get_logger().info(f"Sending {scraperArmFrame}")
        self.get_logger().info(f"Sending {scraperScoopFrame}")
        self.get_logger().info(f"Sending {scraperBucketFrame}")
        try:
            self.bus.send(scraperArmFrame)
            self.bus.send(scraperScoopFrame)
            # self.bus.send(scraperBucketFrame)

        except Exception as e:
            print(e)

    
    def deadline_callback(self, _info):
        # Set all speeds to 0
        self.get_logger().warning("200ms Callback deadline missed")
        self.scraper_stop_state()

    def arm_stop_state(self):
        """
        Stops the scraper arm motor
        """
        self.scraper_arm_velocity = 0
        self.scraper_arm_direction = self.SCRAPER_ARM_FORWARDS

    def scoop_stop_state(self):
        """
        Stops the scraper scoop motor
        """
        self.scraper_scoop_velocity = 0
        self.scraper_scoop_direction = self.SCRAPER_SCOOP_FORWARDS

    def bucket_stop_state(self):
        """
        Stops the scraper bucket motor
        """
        self.scraper_bucket_velocity = 0
        self.scraper_bucket_direction = self.SCRAPER_BUCKET_OPEN

    def scraper_stop_state(self):
        """
        Stops all scraper motors
        """
        self.arm_stop_state()
        self.scoop_stop_state()
        self.bucket_stop_state()

    def check_joystick_lock(self):
        """
        Checks if the joysticks are locked
        Stops all scraper motors if locked
        :return: True if locked, False otherwise
        """
        if self.joystick_lock:
            self.get_logger().info("Joysticks LOCKED!")
            self.scraper_stop_state()
            return True
        return False
    
    def check_scraper_lock(self):
        """
        Checks if the scraper is locked
        Stops all scraper motors if locked
        :return: True if locked, False otherwise
        """
        if self.scraper_lock:
            self.get_logger().info("Scraper LOCKED!")
            self.scraper_stop_state()
            return True
        return False
    
    def update_joystick_lock(self, joystick_l: InputJoystick):
        """
        Updates the joystick lock state
            L2 button = LOCK
            L5 button = UNLOCK
        """
        # Joysticks lock if botton L2 button is pressed on the left joystick
        if joystick_l.btn_bottom_l2_state >= 1 and not self.joystick_lock:
            self.get_logger().info("Joysticks Locked")
            self.joystick_lock = True

        # joysticks unlocked if bottom L5 button is pressed on the left joystick
        if joystick_l.btn_bottom_l5_state >= 1 and self.joystick_lock:
            self.get_logger().info("Joysticks Unlocked")
            self.joystick_lock = False

    def update_scraper_lock(self, joystick_r: InputJoystick):
        """
        Updates the scraper lock state
            R4 button = LOCK
            R1 button = UNLOCK
        """
        # scraper is locked when bottom r4 button is pressed on the right joystick
        if joystick_r.btn_bottom_r4_state >= 1 and not self.scraper_lock:
            self.get_logger().info("Scraper OFF")
            self.scraper_lock = True

        # scraper is unlocked when bottom r1 button is pressed on the right joystick
        if joystick_r.btn_bottom_r1_state >= 1 and self.scraper_lock:
            self.get_logger().info("Scraper ON")
            self.scraper_lock = False

    def update_arm(self, joystick_l: InputJoystick):
        """
        Updates the scraper arm motor
            Left Joystick stick x-axis = direction / velocity
        """
        self.scraper_arm_velocity = abs(int(self.get_parameter(self.SCRAPER_ARM_MAX_VEL_PARAM).value * joystick_l.ax_stick_x))
        self.scraper_arm_direction = self.SCRAPER_ARM_FORWARDS if joystick_l.ax_stick_x >= 0 else self.SCRAPER_ARM_BACKWARDS

    def update_scoop(self, joystick_r: InputJoystick):
        """
        Updates the scraper scoop motor
            Right Joystick stick x-axis = direction / velocity
        """
        self.scraper_scoop_velocity = abs(int(self.get_parameter(self.SCRAPER_SCOOP_MAX_VEL_PARAM).value * joystick_r.ax_stick_x))
        self.scraper_scoop_direction = self.SCRAPER_SCOOP_FORWARDS if joystick_r.ax_stick_x >= 0 else self.SCRAPER_SCOOP_BACKWARDS

    def update_bucket(self, joystick_l: InputJoystick):
        """
        Updates the scraper bucket motor
            Thumb L button = OPEN
            Thumb R button = CLOSE
        """
        if joystick_l._btn_thumb_l_state >= 1:
            self.get_logger().info("Bucket OPEN")
            self.scraper_bucket_direction = self.SCRAPER_BUCKET_OPEN
            self.scraper_bucket_velocity = self.get_parameter(self.SCRAPER_SCOOP_MAX_VEL_PARAM).value
        elif joystick_l._btn_thumb_r_state >= 1:
            self.get_logger().info("Bucket CLOSE")
            self.scraper_bucket_direction = self.SCRAPER_BUCKET_CLOSE
            self.scraper_bucket_velocity = self.get_parameter(self.SCRAPER_SCOOP_MAX_VEL_PARAM).value
        else:
            self.bucket_stop_state()


    def joystick_l_callback(self, msg: InputJoystick):
        """
        Updates the classes internal msg state
        :return: None
        """
        self.get_logger().info("called l")

        joystick_l = msg
    
        self.update_joystick_lock(joystick_l)

        if self.check_joystick_lock() or self.check_scraper_lock():
            return
        
        # Update the inputs
        self.update_arm(joystick_l)
        self.update_bucket(joystick_l)


    def joystick_r_callback(self, msg: InputJoystick):
        """
        Updates the classes internal msg state
        :param msg: core.msg.InputJoystick message from the subscriber callback
        :return: None
        """
        self.get_logger().info("called r")

        joystick_r = msg

        if self.check_joystick_lock():
            return

        self.update_scraper_lock(joystick_r)

        if self.check_scraper_lock():
            return
        
        # Update the inputs
        self.update_scoop(joystick_r)
    
        
    def get_can_commands(self):
        """
        Construct the can commands for the scraper
        """
        card = self.get_parameter(self.CARD_TYPE_PARAM).value
        if card == Controller.CMD.value:
            scraper_arm_data_value = self.scraper_arm_direction * self.scraper_arm_velocity
            scraper_scoop_data_value = self.scraper_scoop_direction * self.scraper_scoop_velocity
            scraper_bucket_data_value = self.scraper_bucket_direction * self.scraper_bucket_velocity

            # pack the values into a list of bytes
            # packing int into 2 bytes big endian
            scraper_arm_data = list(pack('>h', int(scraper_arm_data_value)))
            scraper_scoop_data = list(pack('>h', int(scraper_scoop_data_value)))
            scraper_bucket_data = list(pack('>h', int(scraper_bucket_data_value)))
        else:
            scraper_arm_data = [self.scraper_arm_direction, self.scraper_arm_velocity]
            scraper_scoop_data = [self.scraper_scoop_direction, self.scraper_scoop_velocity]
            scraper_bucket_data = [self.scraper_bucket_direction, self.scraper_bucket_velocity]


        

        return scraper_arm_data, scraper_scoop_data, scraper_bucket_data


def main():
    rclpy.init()
    scraper = ScraperNode()
    rclpy.spin(scraper)
    rclpy.shutdown()


if __name__ == "__main__":
    main()
