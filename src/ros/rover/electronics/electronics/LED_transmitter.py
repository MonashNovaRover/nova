#!/usr/bin/python3

"""
NOVA ROVER TEAM
This script is a ros service that handles communicating commands to the LED Lights.
Authors: Max Tory and Marcel Masque
Last Modified: 12/05/2024 By Victor Bartlinski
"""

import rclpy
from rclpy.node import Node
from std_srvs.srv import Trigger
from coms_utils.can_interface import CANTransmitter
import time
from enum import Enum, IntEnum

"""
Goal 1: get a request in the service, and execute
a callback function setting the colours.
     - for now is printed out to screen 
     - can bus not implemented
"""

class ControlState(Enum):
  AUTONOMOUS = 2
  MANUAL = 3


class AutonomousState(Enum):
  ACTIVE = 4
  SUCCESS = 5


class LedColor(IntEnum):
  RED = 0x091
  GREEN = 0x092
  BLUE = 0x093


class CanLEDCommunicator:
  """
  Handles communication with LED
  """
  def __init__(self):
    self.transmitter = CANTransmitter(
      channel='can0',  # Can channel to transmit on
      arbitration_id=0x091,  # ID for red can trasmitter
    )

  def set_led(self, colour, intensity):
    """
    Sends data to CAN bus to actually activate the LED array
    """
    self.transmitter.arbitration_id = colour  # send to the desired colour LED
    packed_data = int.to_bytes(intensity, 1, "big")
    ret = self.transmitter.transmit(packed_data)

    return ret  # for informing of errors

  def turn_off(self):
    """
    Tells a given colour line to display 0 intensity
    """
    self.set_led(LedColor.RED, 0)
    # self.set_led(LedColor.GREEN, 0)
    # self.set_led(LedColor.BLUE, 0)


class LEDTransmitter(Node):
  """
  Update the LED via service request.
  """
  def __init__(self):
    super().__init__('LED')
    self.can_communicator = CanLEDCommunicator()

    self.control_state = ControlState.AUTONOMOUS
    self.autonomous_state = AutonomousState.ACTIVE

    # Services to handle autonomous state
    self.auto_active_srv = self.create_service(Trigger, 'LED/auto_active', self.cb_auto_active)
    self.auto_success_srv = self.create_service(Trigger, 'LED/auto_success', self.cb_auto_success)
    self.manual_active_srv = self.create_service(Trigger, 'LED/manual_active', self.cb_manual_active)

    self.qos_time = 200
    self.flash_timer = self.create_timer(0.5, self.cb_flash)
    self.flash_counter = 1  # 1 = on, 0 = off
    self.most_recent_update = time.perf_counter()
    # self.display()

    self.can_communicator.turn_off()

  def cb_auto_active(self, request, response):
    """
    Turns the LEDs solid red to indicate the rover entering autonomous mode
    """
    self.get_logger().info("Entering active autonomous mode: LEDs solid red")
    response.success = True
    self.control_state = ControlState.AUTONOMOUS
    self.autonomous_state = AutonomousState.ACTIVE
    self.display()
    return response

  def cb_auto_success(self, request, response):
    """
    Turns the LEDs flashing green to indicate the rover has successfully reached an autonomous goal
    """
    self.get_logger().info("Successfully reached goal: LEDs flashing green")
    response.success = True
    self.control_state = ControlState.AUTONOMOUS
    self.autonomous_state = AutonomousState.SUCCESS
    self.display()
    return response

  def cb_manual_active(self, request, response):
    """
    Turns the LEDs solid blue to indicate the rover entering manual mode
    """
    self.get_logger().info("Entering active manual mode: LEDs solid blue")
    response.success = True
    self.control_state = ControlState.MANUAL
    self.display()
    return response

  def get_color(self):
    """
    Returns the LED colour and intensity [0, 255] to be displayed depending on the current state
    """
    if self.control_state == ControlState.AUTONOMOUS:
      if self.autonomous_state == AutonomousState.SUCCESS:
        # Always flash in Success state, to make sure we get points when we finish a task
        return LedColor.GREEN, 255
      else:
        # In autonomous mode we always display red
        return LedColor.RED, 255
    else:
      # we are in manual mode, so solid blue
      return LedColor.BLUE, 255

  def do_flash(self):
    """
    Returns true if the LEDs should flash in the current state, otherwise false
    """
    if self.control_state == ControlState.AUTONOMOUS:
      if self.autonomous_state == AutonomousState.SUCCESS:
        # Always flash in Success state, to make sure we get points when we finish a task
        return True
      else:
        # We are in active autonomous mode = solid red, so no flashing
        return False
    else:
      # we are in manual mode, so no flashing
      return False

  def cb_flash(self):
    """
    Display a colour based on the mode of the rover either continuous or flashing
    """
    if not self.do_flash():
      return   # don't care about non-flashing modes

    if self.flash_counter == 0: 
      self.can_communicator.turn_off()
    else:
      self.display()
    
    self.flash_counter = (self.flash_counter + 1) % 2

  def display(self):
    """
    Displays a colour based on the current mode of the Rover
    """
    # get colour and brightness
    colour_info = self.get_color()
    # self.can_communicator.turn_off()
    self.can_communicator.set_led(*colour_info)


def main(args=None):
  rclpy.init(args=args)
  node = LEDTransmitter()
  rclpy.spin(node)
  node.destroy_node()
  rclpy.shutdown()


if __name__ == '__main__':
  main()
