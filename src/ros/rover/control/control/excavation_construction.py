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
import jcan

# example of how to import a custom message type
from core.msg import InputJoystick

# an example of how to import a standard message type
from std_msgs.msg import String


class ExcavationConstructionNode(Node):

    def __init__(self):
        super().__init__("excavation_construction")

        self.tile_placer_activated = False
        # self.joystick_locked = True
        self.scraper_arm_direction = 1
        self.scraper_arm_velocity = 1
        self.scraper_scoop_velocity = 0
        self.scraper_scoop_direction = 3

        print("calls")
        # TODO: figure out why its not calling the subscriber callback functions
        # way-point publisher publishes a bunch of waypoints at once (hence using the 2D map datatype
        self.joystick_l_sub = self.create_subscription(InputJoystick, "/control/input_joystick_l", self.joystick_l_callback, 10)
        # current state of internal message
        print("attemps")
        self.joystick_r_sub = self.create_subscription(InputJoystick, "/control/input_joystick_r", self.joystick_r_callback, 10)

        self.bus = jcan.Bus()
        # self.bus.open(self.get_parameter("canbus").value)
        self.bus.open("vcan0")



        # if self.tile_placer_activated and not self.joystick_locked:
        if self.tile_placer_activated:
            # The list of values will be cast to uint8's by JCAN library - so be careful to double check the values!
            tile_placer_commands = self.get_tile_placer_can_commands()
            tilePlacerFrame = jcan.Frame(0x0A0, tile_placer_commands)
            print(f"Sending {tilePlacerFrame}")
            self.bus.send(tilePlacerFrame)

        # elif (not self.joystick_locked):
        else:
            # The list of values will be cast to uint8's by JCAN library - so be careful to double check the values!

            scraper_arm_commands, scraper_scoop_commands = self.get_scraper_can_commands()
            # print(scraper_arm_commands)

            scraperArmFrame = jcan.Frame(0x0A0, scraper_arm_commands)
            print(f"Sending {scraperArmFrame}")
            self.bus.send(scraperArmFrame)

            # The list of values will be cast to uint8's by JCAN library - so be careful to double check the values!
            scraperScoopFrame = jcan.Frame(0x0A0, scraper_scoop_commands)
            print(f"Sending {scraperScoopFrame}")
            self.bus.send(scraperScoopFrame)

    def joystick_l_callback(self, msg):
        """
        Updates the classes internal msg state
        :param msg: core.msg.RoverPose message from the subscriber callback
        :return: None
        """
        print("called l")

        joystick_l = msg

        # # Joysticks lock
        # if joystick_l.btn_bottom_l2_state == 1 :
        #     print("Joysticks locked")
        #     self.joystick_locked = True

        # if joystick_l.btn_bottom_l5_state == 1:
        #     print("Joysticks Unlocked")
        #     self.joystick_locked = False

        # Update the inputs
        self.scraper_arm_velocity = abs(int (255 * joystick_l.ax_stick_x) )
        self.scraper_arm_direction = 1 if joystick_l.ax_stick_x >= 0 else 2

    def joystick_r_callback(self, msg):
        """
        Updates the classes internal msg state
        :param msg: core.msg.RoverPose message from the subscriber callback
        :return: None
        """
        print("called r")

        joystick_r = msg

        # Joysticks lock
        if joystick_r.btn_thumb_l_state == 1 or joystick_r.btn_thumb_d_state == 1:
            self.tile_placer_activated = True
            # Update the inputs
            self.scraper_scoop_velocity = 0
            self.scraper_arm_velocity = 0
            self.tile_placer_velocity = abs( int( 255 * joystick_r.ax_stick_x ) )
            self.scraper_arm_velocity = 0
            self.tile_placer_direction = 5 if joystick_r.ax_stick_x >= 0 else 6
        else:
            self.tile_placer_activated = False
            # Update the inputs
            self.scraper_scoop_velocity = abs( int (255 * joystick_r.ax_stick_x) )
            self.tile_placer_velocity = 0
            self.scraper_scoop_direction = 3 if joystick_r.ax_stick_x >= 0 else 4

    def get_tile_placer_can_commands(self):
        tile_placer_data = [0]
        tile_placer_data.append(self.tile_placer_direction)        
        tile_placer_data.append(self.tile_placer_velocity << 4)
        tile_placer_data.append(self.tile_placer_velocity & 0x0F)
        return tile_placer_data

    def get_scraper_can_commands(self):
        scraper_arm_data = [0]
        scraper_arm_data.append(self.scraper_arm_direction)       
        scraper_arm_data.append(self.scraper_arm_velocity << 4)
        scraper_arm_data.append(self.scraper_arm_velocity & 0x0F)

        scraper_scoop_data = [0]
        scraper_scoop_data.append(self.scraper_scoop_direction)        
        scraper_scoop_data.append(self.scraper_scoop_velocity << 4)
        scraper_scoop_data.append(self.scraper_scoop_velocity & 0x0F)

        return scraper_arm_data, scraper_scoop_data


def main():
    rclpy.init()
    excavation_construction = ExcavationConstructionNode()
    rclpy.spin(excavation_construction)
    rclpy.shutdown()


if __name__ == "__main__":
    main()
