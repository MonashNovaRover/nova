#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: A template ROS node
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: path_planner_node
TOPICS:
  - subscriber: /template/subscriber [RoverPose]
  - publisher: /template/publisher [String]
SERVICES:
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:
AUTHOR(S):	
CREATION:	08/03/2022
EDITED:		08/03/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - change all the template artefacts
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

        self.get_logger().set_level(logging.DEBUG)
        self.param_can = self.declare_parameter("can_bus", "can0").value
        self.tile_placer_activated = False
        # Initially all motors spin forward with 0 velocity
        self.scraper_arm_direction = 0x3
        self.scraper_arm_velocity = 0
        self.scraper_scoop_velocity = 0
        self.scraper_scoop_direction = 0x5
        self.tile_placer_direction = 0x1
        self.tile_placer_velocity = 0


        deadline = Duration(nanoseconds=2e8)        
        events = SubscriptionEventCallbacks(deadline=self.deadline_callback)
        self.qos = QoSProfile(reliability=QoSReliabilityPolicy.BEST_EFFORT, depth=1, deadline=deadline)

        self.tile_placer_id_forwards = 0x2
        self.tile_placer_id_backwards = 0x1
        self.scraper_arm_id_forwards = 0x4
        self.scraper_arm_id_backwards = 0x3
        self.scraper_scoop_id_forwards = 0x6
        self.scraper_scoop_id_backwards = 0x5


        # TODO: figure out why its not calling the subscriber callback functions
        # way-point publisher publishes a bunch of waypoints at once (hence using the 2D map datatype
        self.joystick_l_sub = self.create_subscription(InputJoystick, "/control/input_joystick_l", self.joystick_l_callback, self.qos, event_callbacks=events)
        # current state of internal message

        self.joystick_r_sub = self.create_subscription(InputJoystick, "/control/input_joystick_r", self.joystick_r_callback, self.qos, event_callbacks=events)

        self.bus = jcan.Bus()
        # self.bus.open(self.get_parameter("canbus").value)
        self.bus.open(self.param_can)
        self.timer_jcan = self.create_timer(0.05, self.callback_send_can_commands)


    def callback_send_can_commands(self):
        # if self.tile_placer_activated and not self.joystick_locked:
        # if self.tile_placer_activated:
        # The list of values will be cast to uint8's by JCAN library - so be careful to double check the values!
        tile_placer_commands = self.get_tile_placer_can_commands()
        scraper_arm_commands, scraper_scoop_commands = self.get_scraper_can_commands()
        tilePlacerFrame = jcan.Frame(0x0A0, tile_placer_commands)
        scraperArmFrame = jcan.Frame(0x0A0, scraper_arm_commands)
        scraperScoopFrame = jcan.Frame(0x0A0, scraper_scoop_commands)

        self.get_logger().debug(f"Sending {scraperArmFrame}")
        self.get_logger().debug(f"Sending {scraperScoopFrame}")
        self.get_logger().debug(f"Sending {tilePlacerFrame}")
        try:
            self.bus.send(tilePlacerFrame)
            self.bus.send(scraperArmFrame)
            self.bus.send(scraperScoopFrame)

        except Exception as e:
            print(e)

    
    def deadline_callback(self):
        # Set all speeds to 0

        self.scraper_arm_velocity = 0
        self.scraper_arm_direction = self.scraper_arm_id_forwards
        self.scraper_scoop_velocity = 0
        self.scraper_scoop_direction = self.scraper_scoop_id_forwards
        self.tile_placer_velocity = 0
        self.tile_placer_direction = self.tile_placer_id_forwards


    def joystick_l_callback(self, msg):
        """
        Updates the classes internal msg state
        :param msg: core.msg.RoverPose message from the subscriber callback
        :return: None
        """
        self.get_logger().debug("called l")

        joystick_l = msg

        # # Joysticks lock
        # if joystick_l.btn_bottom_l2_state == 1 :
        #     print("Joysticks locked")
        #     self.joystick_locked = True

        # if joystick_l.btn_bottom_l5_state == 1:
        #     print("Joysticks Unlocked")
        #     self.joystick_locked = False

        # Update the inputs
        self.scraper_arm_velocity = abs(int (200 * joystick_l.ax_stick_x) )
        self.scraper_arm_direction = self.scraper_arm_id_forwards if joystick_l.ax_stick_x >= 0 else self.scraper_arm_id_backwards

    def joystick_r_callback(self, msg):
        """
        Updates the classes internal msg state
        :param msg: core.msg.RoverPose message from the subscriber callback
        :return: None
        """
        self.get_logger().debug("called r")

        joystick_r = msg

        if joystick_r.btn_thumb_l_state >= 1 or joystick_r.btn_thumb_d_state >= 1:
            self.get_logger().debug("using tile placer!")
            self.tile_placer_activated = True
            # Update the inputs

            self.tile_placer_velocity = abs( int( 200 * joystick_r.ax_stick_x ) )
            self.tile_placer_direction = self.tile_placer_id_forwards if joystick_r.ax_stick_x >= 0 else self.tile_placer_id_backwards

            # set scraper velocities to 0
            self.scraper_scoop_velocity = 0
            self.scraper_arm_velocity = 0
            self.scraper_arm_direction = self.scraper_arm_id_forwards
            self.scraper_scoop_direction = self.scraper_scoop_id_forwards
        else:
            self.get_logger().debug("using scraper!")
            self.tile_placer_activated = False
            # Update the inputs
            self.scraper_scoop_velocity = abs( int (255 * joystick_r.ax_stick_x) )
            self.scraper_scoop_direction = self.scraper_scoop_id_forwards if joystick_r.ax_stick_x >= 0 else self.scraper_scoop_id_backwards

            # set tile placer velocities to 0
            self.tile_placer_velocity = 0
            self.tile_placer_direction = self.tile_placer_id_forwards

    def get_tile_placer_can_commands(self):
        tile_placer_data = []
        tile_placer_data.append(self.tile_placer_direction)        
        tile_placer_data.append(self.tile_placer_velocity)

        # tile_placer_data.append(self.tile_placer_velocity & 0x0F)
        return tile_placer_data

    def get_scraper_can_commands(self):
        scraper_arm_data = []
        scraper_arm_data.append(self.scraper_arm_direction)       
        scraper_arm_data.append(self.scraper_arm_velocity)
        # scraper_arm_data.append(self.scraper_arm_velocity & 0x0F)

        scraper_scoop_data = []
        scraper_scoop_data.append(self.scraper_scoop_direction)        
        scraper_scoop_data.append(self.scraper_scoop_velocity)
        # scraper_scoop_data.append(self.scraper_scoop_velocity & 0x0F)

        return scraper_arm_data, scraper_scoop_data


def main():
    rclpy.init()
    excavation_construction = ExcavationConstructionNode()
    rclpy.spin(excavation_construction)
    rclpy.shutdown()


if __name__ == "__main__":
    main()
