#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Control Scraper using Joysticks
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: scraper
TOPICS:
  - subscriber: /inputs/input_joystick_l [InputJoystick]
  - subscriber: /inputs/input_joystick_r [InputJoystick]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: control
COMMAND: ros2 run control scraper.py
RUN ON: Rover
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
AUTHOR(S):	Tristan Clark
CREATION:	02/02/2024
EDITED:		09/03/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

"""
from python_control_old.control_classes import Card, Direction, JonoCardController, OneAxisControl, OneAxisControlLimits, CMDCardController
import rclpy, jcan, logging
from rclpy.node import Node
from rclpy.qos import QoSReliabilityPolicy, QoSProfile
from rclpy.subscription import SubscriptionEventCallbacks
from rclpy.duration import Duration

# import the joystick ROS message we are listening to
from input_interfaces.msg import InputJoystick



class ScraperNode(Node):
    # can bus
    CAN_BUS = "can0"
    # card IDs
    CARD = Card.CMD
    # only used when card = CMD, works with QCMDs
    # 0x0C? ports can handle slightly higher current on QCMDs, 0x0D? ports are slightly weaker
    CMD_ID_ARM = 0x0C1
    CMD_ID_SCOOP = 0x0D1
    CMD_ID_BUCKET = 0x0C2
    # only used when card = JONO
    JONO_ID_ARM = 0x0A0
    JONO_ID_SCOOP = 0x0A0
    JONO_ID_BUCKET = 0x0A0
    JONO_ID_LIMIT_SWITCH = 0x4A2
    # jono commands
    JONO_ARM_FORWARDS = 0x04
    JONO_ARM_BACKWARDS = 0x03
    JONO_SCOOP_FORWARDS = 0x06
    JONO_SCOOP_BACKWARDS = 0x05
    JONO_BUCKET_OPEN = 0x01
    JONO_BUCKET_CLOSE = 0x02

    # directions
    ARM_FORWARDS = Direction.POSITIVE
    ARM_BACKWARDS = Direction.NEGATIVE
    SCOOP_FORWARDS = Direction.POSITIVE
    SCOOP_BACKWARDS = Direction.NEGATIVE
    BUCKET_CLOSE = Direction.POSITIVE
    BUCKET_OPEN = Direction.NEGATIVE
  

    # max_velocity percent
    ARM_MAX_VELOCITY_PERCENT = 1.0
    SCOOP_MAX_VELOCITY_PERCENT = 1.0
    BUCKET_MAX_VELOCITY_PERCENT = 0.6

    # limit switch
    BUCKET_LIMIT_SWITCH_CLOSED = 0x01
    LIMIT_SWITCH_CLEAR = 0x00
    LIMIT_SWITCH_HIT = 0xFF # used to be 0x01

    # ROS param names
    CAN_BUS_PARAM = "can_bus"
    CARD_TYPE_PARAM = "card_type"
    ARM_MAX_VEL_PERCENT_PARAM = "arm_max_vel_percent"
    SCOOP_MAX_VEL_PERCENT_PARAM = "scoop_max_vel_percent"
    BUCKET_MAX_VEL_PERCENT_PARAM = "bucket_max_vel_percent"
    

    def __init__(self):
        super().__init__("scraper")

        self.get_logger().set_level(logging.INFO)
        self.get_logger().info("Scraper starting")

        # Setting ROS parameters
        # This is done so that the parameters can be changed during runtime if desired
        self.declare_parameter(self.CAN_BUS_PARAM, self.CAN_BUS)
        self.declare_parameter(self.CARD_TYPE_PARAM, self.CARD.value)

        self.declare_parameter(self.ARM_MAX_VEL_PERCENT_PARAM, self.ARM_MAX_VELOCITY_PERCENT)
        self.declare_parameter(self.SCOOP_MAX_VEL_PERCENT_PARAM, self.SCOOP_MAX_VELOCITY_PERCENT)
        self.declare_parameter(self.BUCKET_MAX_VEL_PERCENT_PARAM, self.BUCKET_MAX_VELOCITY_PERCENT)


        # Create the custom control classes
        # Arm control
        self.arm = OneAxisControl(
            max_percent=self.get_parameter(self.ARM_MAX_VEL_PERCENT_PARAM).value
        )
        # Scoop control
        self.scoop = OneAxisControl(
            max_percent=self.get_parameter(self.SCOOP_MAX_VEL_PERCENT_PARAM).value
        )
        # Bucket control
        self.bucket = OneAxisControlLimits(
            max_percent=self.get_parameter(self.BUCKET_MAX_VEL_PERCENT_PARAM).value
        )

        self.velocity_multiplier = 0.0

        if self.get_parameter(self.CARD_TYPE_PARAM).value == Card.JONO.value:
            self.setup_jono_controllers()
        elif self.get_parameter(self.CARD_TYPE_PARAM).value == Card.CMD.value:
            self.setup_cmd_controllers()
        else:
            raise ValueError(f"Unknown card type: {self.get_parameter(self.CARD_TYPE_PARAM).value}")

        # Locks the joysticks
        self.joystick_lock = True

        # Set up the QoS profile and deadline callback
        deadline = Duration(nanoseconds=2e8)        
        events = SubscriptionEventCallbacks(deadline=self.deadline_callback)
        self.qos = QoSProfile(reliability=QoSReliabilityPolicy.BEST_EFFORT, depth=1, deadline=deadline)

        # Create the joystick subscribers
        self.joystick_l_sub = self.create_subscription(InputJoystick, "/inputs/input_joystick_l", self.joystick_l_callback, self.qos, event_callbacks=events)
        self.joystick_r_sub = self.create_subscription(InputJoystick, "/inputs/input_joystick_r", self.joystick_r_callback, self.qos, event_callbacks=events)
        

        # Create the CAN bus
        self.bus = jcan.Bus()
        self.bus.set_id_filter([self.JONO_ID_LIMIT_SWITCH])

        self.bus.add_callback(self.JONO_ID_LIMIT_SWITCH, self.callback_receive_can_limit_switch)

        self.bus.open(self.get_parameter(self.CAN_BUS_PARAM).value)
        self.timer_jcan = self.create_timer(0.05, self.callback_send_can_commands)

        self.get_logger().info(f"Scraper started on {self.get_parameter(self.CAN_BUS_PARAM).value} using {self.get_parameter(self.CARD_TYPE_PARAM).value} card type")
        self.get_logger().info("Joysticks Locked")

    def setup_jono_controllers(self):
        # Scraper card controllers for Jono Card
        # Arm controller
        self.arm_controller = JonoCardController(
            card_id=self.JONO_ID_ARM, 
            pos_command=self.JONO_ARM_FORWARDS, 
            neg_command=self.JONO_ARM_BACKWARDS, 
            control=self.arm
        )
        # Scoop controller
        self.scoop_controller = JonoCardController(
            card_id=self.JONO_ID_SCOOP, 
            pos_command=self.JONO_SCOOP_FORWARDS, 
            neg_command=self.JONO_SCOOP_BACKWARDS, 
            control=self.scoop
        )
        # Bucket controller
        self.bucket_controller = JonoCardController(
            card_id=self.JONO_ID_BUCKET, 
            pos_command=self.JONO_BUCKET_OPEN, 
            neg_command=self.JONO_BUCKET_CLOSE, 
            control=self.bucket
        )
    
    def setup_cmd_controllers(self):
        # Scraper card controllers for CMD Card
        # Arm controller
        self.arm_controller = CMDCardController(
            card_id=self.CMD_ID_ARM, 
            control=self.arm
        )
        # Scoop controller
        self.scoop_controller = CMDCardController(
            card_id=self.CMD_ID_SCOOP, 
            control=self.scoop
        )
        # Bucket controller
        self.bucket_controller = CMDCardController(
            card_id=self.CMD_ID_BUCKET, 
            control=self.bucket
        )


    def callback_receive_can_limit_switch(self, frame: jcan.Frame):
        """
        Callback for when the limit switch is hit
        """
        try:
            self.get_logger().debug(f"Received {frame.id} {frame.data}")

            if frame.id == self.JONO_ID_LIMIT_SWITCH and frame.data[0] == self.BUCKET_LIMIT_SWITCH_CLOSED:
                if frame.data[1] == self.LIMIT_SWITCH_CLEAR:
                    self.bucket.update_limit_pos(False)
                else:
                    self.get_logger().debug("Limit switch hit")
                    self.bucket.update_limit_pos(True)
            else:
                self.get_logger().warn("Received unknown frame")

        except Exception as e:
            self.get_logger().error(f"Error receiving CAN message: {e}")
                   


    def callback_send_can_commands(self):
        """
        Take current internal state and publish over CAN
        Sends can commands for scraper scoop, scraper arm together
        """
        # The list of values will be cast to uint8's by JCAN library - so be careful to double check the values!
        armFrame = self.arm_controller.get_frame()
        scoopFrame = self.scoop_controller.get_frame()
        bucketFrame = self.bucket_controller.get_frame()

        self.get_logger().debug(f"Sending {armFrame}")
        self.get_logger().debug(f"Sending {scoopFrame}")
        self.get_logger().debug(f"Sending {bucketFrame}")
        try:
            self.bus.send(armFrame)
            self.bus.send(scoopFrame)
            self.bus.send(bucketFrame)

        except Exception as e:
            print(e)

    
    def deadline_callback(self, _info):
        # Set all speeds to 0
        self.get_logger().warning("200ms Callback deadline missed")
        self.scraper_stop()

    def scraper_stop(self):
        """
        Stops all scraper motors
        """
        self.arm.stop()
        self.scoop.stop()
        self.bucket.stop()

    def check_joystick_lock(self):
        """
        Checks if the joysticks are locked
        Stops all scraper motors if locked
        :return: True if locked, False otherwise
        """
        if self.joystick_lock:
            self.scraper_stop()
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


    def update_arm(self, joystick_l: InputJoystick):
        """
        Updates the scraper arm motor
            Left Joystick stick x-axis = direction / velocity
        """
        self.arm.update_velocity(abs(joystick_l.ax_stick_x) * self.velocity_multiplier)
        self.arm.update_direction(self.ARM_FORWARDS if joystick_l.ax_stick_x >= 0 else self.ARM_BACKWARDS)

    def update_scoop(self, joystick_r: InputJoystick):
        """
        Updates the scraper scoop motor
            Right Joystick stick x-axis = direction / velocity
        """
        self.scoop.update_velocity(abs(joystick_r.ax_stick_x) * self.velocity_multiplier)
        self.scoop.update_direction(self.SCOOP_FORWARDS if joystick_r.ax_stick_x >= 0 else self.SCOOP_BACKWARDS)

    def update_bucket(self, joystick_r: InputJoystick):
        """
        Updates the scraper bucket motor
            Left Joystick thumb stick x-axis = direction / velocity
        """
        self.bucket.update_velocity(abs(joystick_r.ax_thumb_x))
        self.bucket.update_direction(self.BUCKET_OPEN if joystick_r.ax_thumb_x >= 0 else self.BUCKET_CLOSE)


    def joystick_l_callback(self, msg: InputJoystick):
        """
        Updates the classes internal msg state
        :return: None
        """
        self.get_logger().debug("called l")

        joystick_l = msg
    
        self.update_joystick_lock(joystick_l)

        if self.check_joystick_lock():
            return
        
        # Update the inputs
        self.update_arm(joystick_l)


    def joystick_r_callback(self, msg: InputJoystick):
        """
        Updates the classes internal msg state
        :param msg: input_interfaces.msg.InputJoystick message from the subscriber callback
        :return: None
        """
        self.get_logger().debug("called r")

        joystick_r = msg

        if self.check_joystick_lock():
            return
        
        # Update the inputs
        self.velocity_multiplier = abs(joystick_r.ax_slider)

        self.update_scoop(joystick_r)
        self.update_bucket(joystick_r)
    

def main():
    rclpy.init()
    scraper = ScraperNode()
    rclpy.spin(scraper)
    rclpy.shutdown()


if __name__ == "__main__":
    main()
