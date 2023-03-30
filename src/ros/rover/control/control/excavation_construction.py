#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: A template ROS node
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: excavation_construction
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
from std_msgs.msg import String
from rclpy.qos import QoSReliabilityPolicy, QoSProfile
from rclpy.subscription import SubscriptionEventCallbacks
from rclpy.duration import Duration


class ExcavationConstructionNode(Node):

    def __init__(self):
        super().__init__("excavation_construction")

        self.get_logger().set_level(logging.WARN)
        self.param_can = self.declare_parameter("can_bus", "can0").value
        self.param_scraper_arm_multiplier = self.declare_parameter("scraper_arm_multiplier", 200).value
        self.param_scraper_scoop_multiplier = self.declare_parameter("scraper_scoop_multiplier", 255).value
        self.param_tile_placer_multiplier = self.declare_parameter("tile_placer_multiplier", 200).value

        self.tile_placer_activated = False

        # set motor ids
        self.tile_placer_id_forwards = 0x2
        self.tile_placer_id_backwards = 0x1
        self.scraper_arm_id_forwards = 0x4
        self.scraper_arm_id_backwards = 0x3
        self.scraper_scoop_id_forwards = 0x6
        self.scraper_scoop_id_backwards = 0x5

        # Initially all motors spin backwards with 0 velocity
        self.scraper_arm_direction = self.scraper_arm_id_backwards
        self.scraper_scoop_direction = self.scraper_scoop_id_backwards
        self.tile_placer_direction = self.tile_placer_id_backwards
        self.scraper_arm_velocity = 0
        self.scraper_scoop_velocity = 0
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
        Sends can commands for scraper scoop, scraper arm and tile placer together
        """
        # The list of values will be cast to uint8's by JCAN library - so be careful to double check the values!
        tile_placer_commands = self.get_tile_placer_can_commands()
        scraper_arm_commands, scraper_scoop_commands = self.get_scraper_can_commands()
        tilePlacerFrame = jcan.Frame(0x0A0, tile_placer_commands)
        scraperArmFrame = jcan.Frame(0x0A0, scraper_arm_commands)
        scraperScoopFrame = jcan.Frame(0x0A0, scraper_scoop_commands)

        self.get_logger().info(f"Sending {scraperArmFrame}")
        self.get_logger().info(f"Sending {scraperScoopFrame}")
        self.get_logger().info(f"Sending {tilePlacerFrame}")
        try:
            self.bus.send(tilePlacerFrame)
            self.bus.send(scraperArmFrame)
            self.bus.send(scraperScoopFrame)

        except Exception as e:
            print(e)

    
    def deadline_callback(self, info):
        # Set all speeds to 0
        self.get_logger().warning("200ms Callback deadline missed")
        self.scraper_arm_velocity = 0
        self.scraper_arm_direction = self.scraper_arm_id_forwards
        self.scraper_scoop_velocity = 0
        self.scraper_scoop_direction = self.scraper_scoop_id_forwards
        self.tile_placer_velocity = 0
        self.tile_placer_direction = self.tile_placer_id_forwards


    def joystick_l_callback(self, msg):
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
                self.get_logger().info("Tile Placer Mode")
            else:
                self.get_logger().info("Scraper Mode")
            self.joystick_lock = False

        # Update the inputs
        if not self.joystick_lock and not self.tile_placer_activated:
            self.scraper_arm_velocity = abs(int (self.param_scraper_arm_multiplier * joystick_l.ax_stick_x) )
            self.scraper_arm_direction = self.scraper_arm_id_forwards if joystick_l.ax_stick_x >= 0 else self.scraper_arm_id_backwards
        elif not self.joystick_lock and self.tile_placer_activated:
            # send 0 velocity
            self.scraper_arm_velocity = 0
            self.scraper_arm_direction = self.scraper_arm_id_forwards
        elif self.joystick_lock:
            # if joysticks locked
            self.get_logger().info("joysticks locked!")
            self.scraper_arm_velocity = 0
            self.scraper_arm_direction = self.scraper_arm_id_forwards

    def joystick_r_callback(self, msg):
        """
        Updates the classes internal msg state
        :param msg: core.msg.RoverPose message from the subscriber callback
        :return: None
        """
        self.get_logger().info("called r")

        joystick_r = msg

        # Scraper mode is on
        if joystick_r.btn_bottom_r1_state >= 1 and self.tile_placer_activated:
            self.get_logger().info("Scraper Mode")
            self.tile_placer_activated = False

        # tile placer is unlocked when bottom r4 button is pressed on the right joystick
        # Tile Placer mode is on
        if joystick_r.btn_bottom_r4_state >= 1 and not self.tile_placer_activated:
            self.get_logger().info("Tile Placer Mode")
            self.tile_placer_activated = True

        if not self.joystick_lock and self.tile_placer_activated:
            # Update the inputs
            self.tile_placer_velocity = abs( int( self.param_tile_placer_multiplier * joystick_r.ax_stick_x ) )
            self.tile_placer_direction = self.tile_placer_id_forwards if joystick_r.ax_stick_x >= 0 else self.tile_placer_id_backwards

            # set scraper velocities to 0
            self.scraper_scoop_velocity = 0
            self.scraper_arm_velocity = 0
            self.scraper_arm_direction = self.scraper_arm_id_forwards
            self.scraper_scoop_direction = self.scraper_scoop_id_forwards
        elif not self.joystick_lock and not self.tile_placer_activated:
            # Update the inputs
            self.scraper_scoop_velocity = abs( int (self.param_scraper_scoop_multiplier * joystick_r.ax_stick_x) )
            self.scraper_scoop_direction = self.scraper_scoop_id_forwards if joystick_r.ax_stick_x >= 0 else self.scraper_scoop_id_backwards

            # set tile placer velocities to 0
            self.tile_placer_velocity = 0
            self.tile_placer_direction = self.tile_placer_id_forwards
        else:
            # if joysticks locked
            self.get_logger().info("joysticks locked!")
            self.tile_placer_activated = False
            # set the scoop velocity to 0
            self.scraper_scoop_velocity = 0
            self.scraper_scoop_direction = self.scraper_scoop_id_forwards

            # set tile placer velocity to 0
            self.tile_placer_velocity = 0
            self.tile_placer_direction = self.tile_placer_id_forwards


    def get_tile_placer_can_commands(self):
        tile_placer_data = []
        tile_placer_data.append(self.tile_placer_direction)        
        tile_placer_data.append(self.tile_placer_velocity)

        return tile_placer_data

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
    excavation_construction = ExcavationConstructionNode()
    rclpy.spin(excavation_construction)
    rclpy.shutdown()


if __name__ == "__main__":
    main()
