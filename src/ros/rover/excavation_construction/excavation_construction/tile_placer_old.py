#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Control Tile Placer using Joysticks
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: tile_placer
TOPICS:
  - subscriber: /inputs/input_joystick_l [InputJoystick]
  - subscriber: /inputs/input_joystick_r [InputJoystick]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: control
COMMAND: ros2 run control tile_placer.py
RUN ON: Rover
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
AUTHOR(S):	Tristan Clark
CREATION:	02/02/2024
EDITED:		09/03/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

"""
from python_control_old.control_classes import CMDCardController, Card, Direction, OneAxisControl
import rclpy, jcan, logging
from rclpy.node import Node
from rclpy.qos import QoSReliabilityPolicy, QoSProfile
from rclpy.subscription import SubscriptionEventCallbacks
from rclpy.duration import Duration

# import the joystick ROS message we are listening to
from input_interfaces.msg import InputJoystick

CARD = Card.CMD

class TilePlacerNode(Node):
    # can bus
    CAN_BUS = "can0"
    # card IDs
    # only used when card = CMD, works with QCMDs
    # 0x0C? ports can handle slightly higher current on QCMDs, 0x0D? ports are slightly weaker
    CMD_ID_TILE_PLACER = 0x0D2
    # only used when card = JONO
    JONO_ID_TILE_PLACER = 0x0A0
    # jono commands
    TILE_PLACER_ID_UP = 0x02
    TILE_PLACER_ID_DOWN = 0x01

    # directions
    TILE_PLACER_UP = Direction.POSITIVE
    TILE_PLACER_DOWN = Direction.NEGATIVE

    # max_velocity percent
    TILE_PLACER_MAX_VELOCITY = 0.70

    # ROS param names
    CAN_BUS_PARAM = "can_bus"
    TILE_PLACER_MAX_VEL_PERCENT_PARAM = "tile_placer_max_vel_percent"

    def __init__(self):
        super().__init__("tile_placer")

        self.get_logger().set_level(logging.INFO)
        self.get_logger().info("Tile Placer starting")

        self.declare_parameter(self.CAN_BUS_PARAM, self.CAN_BUS)
        self.declare_parameter(self.TILE_PLACER_MAX_VEL_PERCENT_PARAM, self.TILE_PLACER_MAX_VELOCITY)

        self.tile_placer = OneAxisControl(
            max_percent=self.get_parameter(self.TILE_PLACER_MAX_VEL_PERCENT_PARAM).value
        )

        self.tile_placer_controller = CMDCardController(
            card_id=self.CMD_ID_TILE_PLACER,
            control=self.tile_placer,
        )

        self.velocity_multiplier = 0.0

        self.joystick_lock = True

        deadline = Duration(nanoseconds=2e8)        
        events = SubscriptionEventCallbacks(deadline=self.deadline_callback)
        self.qos = QoSProfile(reliability=QoSReliabilityPolicy.BEST_EFFORT, depth=1, deadline=deadline)

        self.joystick_l_sub = self.create_subscription(InputJoystick, "/inputs/input_joystick_l", self.joystick_l_callback, self.qos, event_callbacks=events)
        self.joystick_r_sub = self.create_subscription(InputJoystick, "/inputs/input_joystick_r", self.joystick_r_callback, self.qos, event_callbacks=events)

        self.bus = jcan.Bus()
        self.bus.open(self.get_parameter(self.CAN_BUS_PARAM).value)
        self.timer_jcan = self.create_timer(0.05, self.callback_send_can_commands)

        self.get_logger().info(f"Tile Placer started on {self.get_parameter(self.CAN_BUS_PARAM).value}")
        self.get_logger().info("Joysticks Locked")


    def callback_send_can_commands(self):
        """Take current internal state and publish over CAN
        Sends can commands for tile placer
        """
        # The list of values will be cast to uint8's by JCAN library - so be careful to double check the values!
        tilePlacerFrame = self.tile_placer_controller.get_frame()

        self.get_logger().debug(f"Sending {tilePlacerFrame}")
        try:
            self.bus.send(tilePlacerFrame)

        except Exception as e:
            print(e)

    
    def deadline_callback(self, info: InputJoystick):
        # Set all speeds to 0
        self.get_logger().warning("200ms Callback deadline missed")
        self.tile_placer_stop()

    def tile_placer_stop(self):
        self.tile_placer.stop()

    def check_joystick_lock(self):
        """
        Checks if the joysticks are locked
        Stops all scraper motors if locked
        :return: True if locked, False otherwise
        """
        if self.joystick_lock:
            self.tile_placer_stop()
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


    def update_tile_placer(self, joystick_r: InputJoystick):
        if joystick_r.btn_thumb_l_state >= 1:
            self.get_logger().debug("Tile Placer UP")
            self.tile_placer.update_direction(self.TILE_PLACER_UP)
            self.tile_placer.update_velocity(self.velocity_multiplier)
        elif joystick_r.btn_thumb_r_state >= 1:
            self.get_logger().debug("Tile Placer DOWN")
            self.tile_placer.update_direction(self.TILE_PLACER_DOWN)
            self.tile_placer.update_velocity(self.velocity_multiplier)
        else:
            self.tile_placer_stop()


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
            
        self.velocity_multiplier = abs(joystick_l.ax_slider)


    def joystick_r_callback(self, msg: InputJoystick):
        """
        Updates the classes internal msg state
        :param msg: input_interfaces.msg.RoverPose message from the subscriber callback
        :return: None
        """
        self.get_logger().debug("called r")

        joystick_r = msg

        if self.check_joystick_lock():
            return
        
        # Update inputs
        self.update_tile_placer(joystick_r)  


def main():
    rclpy.init()
    tile_placer = TilePlacerNode()
    rclpy.spin(tile_placer)
    rclpy.shutdown()


if __name__ == "__main__":
    main()
