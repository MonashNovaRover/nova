#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Control Auger Height and Drill Spin using Joysticks
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: auger
TOPICS:
  - subscriber: /control/input_joystick_l [InputJoystick]
  - subscriber: /control/input_joystick_r [InputJoystick]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:
AUTHOR(S):	Tristan Clark
CREATION:	24/02/2024
EDITED:		24/02/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

"""
from python_control.classes.cards.CardController import CardController
from python_control.classes.limits.Limit import Limit
from python_control.classes.sensors.Sensor import Sensor
import rclpy, jcan, logging
from struct import pack
from rclpy.node import Node
from rclpy.qos import QoSReliabilityPolicy, QoSProfile
from rclpy.subscription import SubscriptionEventCallbacks
from rclpy.duration import Duration

# import the joystick ROS message we are listening to
from core.msg import InputJoystick


class ControllerNode(Node):
    # ROS parameter names
    CAN_BUS_PARAM = "can_bus"


    def __init__(self,  name: str, can_bus: str, logging_level: int = logging.INFO):
        super().__init__(name)

        self.get_logger().set_level(logging_level)
        self.get_logger().info(f"{name} starting")

        self.declare_parameter(self.CAN_BUS_PARAM, can_bus)

        self.joystick_lock = True

        self.controllers : dict[str, CardController] = {}
        self.sensors : dict[str, Sensor] = {}

        deadline = Duration(nanoseconds=2e8)
        events = SubscriptionEventCallbacks(deadline=self.deadline_callback)
        self.qos = QoSProfile(reliability=QoSReliabilityPolicy.BEST_EFFORT, depth=1, deadline=deadline)

        self.joystick_l_sub = self.create_subscription(InputJoystick, "/control/input_joystick_l", self.joystick_l_callback, self.qos, event_callbacks=events)
        self.joystick_r_sub = self.create_subscription(InputJoystick, "/control/input_joystick_r", self.joystick_r_callback, self.qos, event_callbacks=events)

        self.get_logger().info(f"{self.get_name()} started using {self.get_parameter(self.CARD_TYPE_PARAM).value} card type")
        self.get_logger().info("Joysticks Locked")

    def get_can_bus(self):
        return self.get_parameter(self.CAN_BUS_PARAM).value

    def add_controller(self, controller_name: str, controller: CardController):
        self.controllers[controller_name] = controller
        if controller.control.pos_limit is not None:
            pos_limit : Limit = controller.control.pos_limit
            self.add_receive_can_callback(pos_limit.get_frame_id(), pos_limit.frame_callback)
        if controller.control.neg_limit is not None:
            neg_limit : Limit = controller.control.neg_limit
            self.add_receive_can_callback(neg_limit.get_frame_id(), neg_limit.frame_callback)

    def add_sensor(self, sensor_name: str, sensor: Sensor):
        self.sensors[sensor_name] = sensor
        self.add_receive_can_callback(sensor.get_frame_id(), sensor.frame_callback)




    def callback_send_commands(self):
        """Take current internal state and publish over CAN
        Sends can commands for auger and drill together
        """
        try:
            for controller in self.controllers.values():
                frame = controller.get_frame()
                self.get_logger().debug(f"Sending {frame}")
                self.bus.send(frame)

        except Exception as e:
            print(e)

    def deadline_callback(self, _info):
        # Set all speeds to 0
        self.get_logger().warning("200ms Callback deadline missed")
        self.stop_state()

    def stop_state(self):
        """
        Stop all motors
        """
        for controller in self.controllers:
            controller.control.stop()

    # def callback_receive_can_time_of_flight(self, frame: jcan.Frame):
    #     """Receive can feedback for auger limit switches
    #     """
    #     self.get_logger().debug(f"Received {hex(frame.id)} {frame.data}")
    #     for controller in self.controllers:
    #         if frame.id == frame_id:
    #             controller.update_time_of_flight(frame)
    #             return

    #     if len(frame.data) != 2:
    #         self.get_logger().error(f"Time of flight error")
    #         return

    #     if frame.id == self.JONO_ID_TIME_OF_FLIGHT:
       
    #         raw_height = int(frame.data[1] + (frame.data[0] << 8))
    #         height = self.convert_time_of_flight(raw_height)
    #         self.get_logger().debug(f"Raw height: {raw_height}, Converted height: {height}")
    #         if height == 0:
    #             self.get_logger().debug("Time of flight hit bottom")
    #             self.platform.update_limit_neg(True)
    #         else:
    #             self.platform.update_limit_neg(False)

    #         self.publish_time_of_flight(height)
    #     else:
    #         self.get_logger().warn(f"Received unknown frame {frame}")


    def callback_receive_can_feedback(self, frame: jcan.Frame):
        """Receive can feedback for auger limit switches
        """
        self.get_logger().debug(f"Received {hex(frame.id)} {frame.data}")
 
        if frame.id == self.CARD_ID_RECEIVE:
            if frame.data[0] == self.AUGER_LIMIT_SWITCH_TOP:
                if frame.data[1] == self.AUGER_LIMIT_SWITCH_CLEAR:
                    self.top_limit = False
                else:
                    self.get_logger().debug("Top limit switch hit")
                    self.top_limit = True
               
            elif frame.data[0] == self.AUGER_LIMIT_SWITCH_BOTTOM:
                if frame.data[1] == self.AUGER_LIMIT_SWITCH_CLEAR:
                    self.bottom_limit = False
                else:
                    self.get_logger().debug("Bottom limit switch hit")
                    self.bottom_limit = True
        else:
            self.get_logger().warn(f"Received unknown frame {frame}")
        


    def auger_stop_state(self):
        self.auger_velocity = 0
        self.auger_direction = self.AUGER_UP

    def drill_stop_state(self):
        self.drill_velocity = 0
        self.drill_direction = self.DRILL_CLOCKWISE

    
    def deadline_callback(self, _info):
        # Set all speeds to 0
        self.get_logger().warning("200ms Callback deadline missed")
        self.auger_stop_state()
        self.drill_stop_state()

    def check_joystick_lock(self):
        if self.joystick_lock:
            self.auger_stop_state()
            self.drill_stop_state()
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

    def update_auger_height(self, joystick_r: InputJoystick):
        # Auger height direction is determined by the right joystick's x-axis direction
        self.auger_direction = self.AUGER_DOWN if joystick_r.ax_stick_x >= 0 else self.AUGER_UP 

        # Auger velocity is determined by the right joystick's x-axis magnitude
        # If the auger is at the top or bottom limit, the velocity is set to 0
        if ((self.auger_direction == self.AUGER_UP and self.top_limit) or 
            (self.auger_direction == self.AUGER_DOWN and self.bottom_limit)):
            self.auger_velocity = 0
        else:
            self.auger_velocity = abs(int(self.get_parameter(self.AUGER_MAX_VELOCITY_PARAM).value * joystick_r.ax_stick_x))

    def update_drill_spin(self, joystick_r: InputJoystick):
        # Drill spin direction is determined by the right joystick thumb buttons
        # Thumb right = clockwise, Thumb left = counterclockwise
        if joystick_r.btn_thumb_r_state >= 1:
            self.drill_direction = self.DRILL_CLOCKWISE
        elif joystick_r.btn_thumb_l_state >= 1:
            self.drill_direction = self.DRILL_COUNTERCLOCKWISE
        
        # Drill spin velocity is determined by the right joystick trigger
        if joystick_r.btn_thumb_u_state >= 1:
            self.drill_velocity = self.get_parameter(self.DRILL_MAX_VELOCITY_PARAM).value
        else:
            self.drill_velocity = 0      

    def joystick_l_callback(self, msg: InputJoystick):
        """
        Updates the classes internal msg state
        :return: None
        """
        self.get_logger().debug("called l")

        joystick_l = msg

        self.update_joystick_lock(joystick_l)

    



    def joystick_r_callback(self, msg: InputJoystick):
        """
        Updates the classes internal msg state
        :param msg: core.msg.RoverPose message from the subscriber callback
        :return: None
        """
        self.get_logger().debug("called r")

        joystick_r = msg

        if self.check_joystick_lock():
            return

        # Update the inputs

        self.update_auger_height(joystick_r)
        self.update_drill_spin(joystick_r)

          


    # construct the can commands for the auger and drill
    def get_can_commands(self):
        auger_data_value = self.auger_direction * self.auger_velocity
        drill_data_value = self.drill_direction * self.drill_velocity
        self.get_logger().debug(f"Auger: {auger_data_value}, Drill: {drill_data_value}")

        # pack the values into a list of bytes
        # packing int into 2 bytes big endian
        auger_data = list(pack('>h', int(auger_data_value)))
        drill_data = list(pack('>h', int(drill_data_value)))

        return auger_data, drill_data



