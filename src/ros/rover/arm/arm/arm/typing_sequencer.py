#!/usr/bin/env python
'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

ROS action service which runs auto typing, 
    interfaces with the following:
- Keyboard localiser (arm/keyboard_localiser.py)
- Path planner (404 not found)
- Controller switcher (404 not found)
Used for auto typing task at URC
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: typing_sequencer
SERVICES: /type_sequence
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	arm
AUTHOR(S):  Anthony Lew
CREATION:	9/05/2024
EDITED:     9/05/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Add stop functionality to sequencer
 - Integrate Path Planner and Controller Switcher properly
 - Add error handling
 - Test!
 - Integrate with GUI
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''

import time

import rclpy
from rclpy.action import ActionClient
from rclpy.node import Node
from tf2_ros import Buffer, TransformListener
from std_srvs.srv import Trigger
from geometry_msgs.msg import Transform

import tf2_geometry_msgs

# from arm_interfaces.action import PathTo
from arm_interfaces.srv import KeyPosition, TypeSequence

TYPING_SEQUENCER_START = "/type_sequence/start"
TYPING_SEQUENCER_STOP = "/type_sequence/stop"
CONTROLLER_SWITCH_SERVICE = "/controller_manager/switch_controller"
KEY_POSITION_SERVICE = "/arm/keyboard/pub_key_position"
PATH_PLANNER_ACTION = "/move_to_pos"
POKEY_THING_ACTION = "/poke"

class TypingSequencer(Node):

    def __init__(self):
        super().__init__('typing_sequencer')
        self.sequencer_start_server = self.create_service(TypeSequence, TYPING_SEQUENCER_START, self.execute_sequencer)
        self.sequencer_stop_server = self.create_service(Trigger, TYPING_SEQUENCER_STOP, self.stop_sequencer)

        self.is_stopped = False

        # Parameters
        self.keyboard_frame = self.declare_parameter('keyboard_frame', 'keyboard_frame').get_parameter_value().string_value
        self.base_frame = self.declare_parameter('base_frame', 'base_link').get_parameter_value().string_value
        self.ee_frame = self.declare_parameter('ee_frame', 'eebase').get_parameter_value().string_value
        self.ee_dist = self.declare_parameter('arm_hover_dist', 10.0).get_parameter_value().double_value # cm to hover away from keyboard

        # Listen to /tf
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(self.tf_buffer, self)
        self.tf_timeout = self.declare_parameter('timeout', 2.0).get_parameter_value().double_value       # seconds to wait between each check
        self.tf_poll_rate = self.declare_parameter('poll_rate', 10.0).get_parameter_value().double_value  # check frequency in Hz

        # Controller switcher service client
        # TODO: Uncomment once integrated
        # self.cswitcher_client = self.create_client(Trigger, CONTROLLER_SWITCH_SERVICE)
        # while not self.cswitcher_client.wait_for_service(timeout_sec=1.0):
        #     self.get_logger().info(f'{CONTROLLER_SWITCH_SERVICE} service not available, waiting again...')

        # Key localiser service client
        self.kblocaliser_client = self.create_client(KeyPosition, KEY_POSITION_SERVICE)
        while not self.kblocaliser_client.wait_for_service(timeout_sec=1.0):
            self.get_logger().info(f'{KEY_POSITION_SERVICE} service not available, waiting again...')

        # Path planner action client
        # TODO: Uncomment once integrated
        # self.pplanner_client = ActionClient(self, PathTo, PATH_PLANNER_ACTION)
        
        # Pokey Thing action client
        # TODO: Uncomment once integrated
        # self.pokey_client = ActionClient(self, Trigger, POKEY_THING_ACTION)
        self.get_logger().info(f'Sequencer initalised!')

    def send_switch_request(self):
        request = Trigger.Request()
        future = self.cswitcher_client.call_async(request)
        rclpy.spin_until_future_complete(self, future)
        return future.result()

    def send_key_request(self, key, stamp):
        request = KeyPosition.Request()
        request.key = key
        request.stamp = stamp
        future = self.kblocaliser_client.call_async(request)
        rclpy.spin_until_future_complete(self, future)
        return future.result()


    def path_to_tf(self, transform):
        goal_msg = PathTo.Goal()
        goal_msg.transform = transform

        self.pplanner_client.wait_for_server()
        future_response = self.pplanner_client.send_goal_async(goal_msg, feedback_callback=self.handle_pp_feedback)
        rclpy.spin_until_future_complete(self, future_response)
        response = future_response.result()
        if not response.accepted:
            self.get_logger().info('Path planner goal rejected')
            return False
        
        future_result = response.get_result_async()
        rclpy.spin_until_future_complete(self, future_result)
        result = future_result.result().result
        return result

    def handle_pp_feedback(self, feedback_msg):
        feedback = feedback_msg.feedback
        self.get_logger().info('Recieved feedback: {0}'.format(feedback.status))
    
    def do_poke(self):
        goal_msg = Trigger.Goal()

        self.pokey_client.wait_for_server()
        future_response = self.pokey_client.send_goal_async(goal_msg, feedback_callback=self.handle_pt_feedback)
        rclpy.spin_until_future_complete(self, future_response)
        response = future_response.result()
        if not response.accepted:
            self.get_logger().info('Pokey Thing goal rejected')
            return False
        
        future_result = response.get_result_async()
        rclpy.spin_until_future_complete(self, future_result)
        result = future_result.result().result
        return result
    
    def handle_pt_feedback(self, feedback_msg):
        feedback = feedback_msg.feedback
        self.get_logger().info('Recieved feedback: {0}'.format(feedback.status))

    def get_transform_now_from_base_frame(self, target_frame, stamp=None) -> Transform | None:
        self.get_logger().info(f'Getting transform of frame: {target_frame}')
        if stamp is None:
            stamp = self.get_clock().now() 
        start_time = self.get_clock().now()
        while self.get_clock().now() - start_time < rclpy.duration.Duration(seconds=self.timeout):
            if self.tf_buffer.can_transform(target_frame, self.base_frame, stamp):
                transform = self.tf_buffer.lookup_transform(target_frame, self.base_frame, stamp)
                return transform
            time.sleep(1.0/self.poll_rate)
        else:
            self.get_logger().warn(f'Transform of {target_frame} not available after waiting {self.timeout} seconds')
        return None

    def execute_sequencer(self, request, response):
        self.is_stopped = False
        key_sequence = request.sequence
        self.get_logger().info(f'Beginning sequencer with the following sequence: {key_sequence}')

        partial_sequence = []

        # Get position of EE in base link frame (Assumes operators have aligned keyboard with camera)
        # ee_transform = self.get_transform_now_from_base_frame(self.ee_frame)
        # if ee_transform is None:
        #     response.success = False
        #     return response

        # Call controller switcher and switch to "Auto typing mode" (IK only mode)
        # TODO: Integrate the switcher
        # switch_result = self.send_switch_request()
        # if not switch_result.success:
        #     self.get_logger().error(f'Switching Error: {switch_result.message}')
        #     return
        
        # Loop through the keys in the sequence
        for key in key_sequence:
            self.get_logger().info(f'Performing sequence for key: {key}')
            # Get Key transform to be published on /tf by calling keyboard localiser
            stamp = self.get_clock().now().to_msg()
            key_result = self.send_key_request(key, stamp)
            if not key_result.success:
                self.get_logger().error(f'Key Transform Error: {key_result.message}')
                response.success = False
                return response
            
            # Get key transform
            key_frame = key + "_" + self.keyboard_frame
            self.get_logger().info(f'Get key: {key}')
            key_transform = self.get_transform_now_from_base_frame(key_frame, stamp)
            if key_transform is None:
                response.success = False
                return response
            
            # TODO: Integrate and test this section

            # Start action to move to key via path planner
            # TODO: Add error handling
            # self.path_to_tf(key_transform)

            # Activate pokey thing
            # TODO: Add error handling
            # self.do_poke()

            # TODO: Publish feedback to topic for GUI
            # partial_sequence.append(key)
            self.get_logger().info(f'Completed: {partial_sequence}')

            # Move back to starting position
            # TODO: Integrate + Error handling
            # self.path_to_tf(ee_transform)

        # Call controller switcher and switch back to manual mode
        # TODO: Integrate the switcher
        # switch_result = self.send_switch_request()
        # if not switch_result.success:
        #     self.get_logger().error(f'Switching Error: {switch_result.message}')
        #     response.success = False
        #     return response
        self.get_logger().info(f'Sequencer Complete! {partial_sequence}')

        response.success = True
        return response

    def stop_sequencer(self):
        self.is_stopped = True

def main(args=None):
    rclpy.init(args=args)
    node = TypingSequencer()
    rclpy.spin(node)


if __name__ == '__main__':
    main()