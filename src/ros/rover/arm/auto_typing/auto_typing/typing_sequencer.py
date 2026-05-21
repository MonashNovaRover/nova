#!/usr/bin/env python
'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

ROS action service which runs auto typing, 
    interfaces with the following:
- Keyboard localiser (auto_typing/keyboard_localiser.py)
- Path planner (nova_path_planner controller)
Used for auto typing task at URC
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: typing_sequencer
SERVICES: /type_sequence
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	auto_typing
AUTHOR(S):  Anthony Lew
CREATION:	9/05/2024
EDITED:     12/05/2026
EDITED BY:  Binuda Kalugalage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''

import time

import rclpy
from rclpy.executors import MultiThreadedExecutor
from rclpy.action import ActionClient
from rclpy.node import Node
from tf2_ros import Buffer, TransformListener, TransformBroadcaster, TransformStamped
from std_srvs.srv import Trigger
from geometry_msgs.msg import Transform, Pose
#from nova_interfaces.action import EndEffector

import numpy as np
from scipy.spatial.transform import Rotation as R
import threading
import tf2_geometry_msgs

# from arm_interfaces.action import PathTo
from arm_interfaces.msg import SequencerFeedback
from arm_interfaces.srv import KeyPosition, TypeSequence
from nova_interfaces.action import ArmPlanPath, EndEffector

TYPING_SEQUENCER_START = "/type_sequence/start"
TYPING_SEQUENCER_STOP = "/type_sequence/stop"
KEY_POSITION_SERVICE = "/arm/keyboard/pub_key_position"
PATH_PLANNER_ACTION = '/arm/plan_path'
POKEY_THING_ACTION = "/arm/poke"
SEQUENCER_TOPIC = "/arm/sequence"

POKE_FORWARD = 1.0
POKE_BACKWARD = 0.0

class TypingSequencer(Node):

    def __init__(self):
        super().__init__('typing_sequencer')
        self.sequencer_start_server = self.create_service(TypeSequence, TYPING_SEQUENCER_START, self.start_sequencer)
        self.sequencer_stop_server = self.create_service(Trigger, TYPING_SEQUENCER_STOP, self.stop_sequencer)

        self.thread = None
        self.stop_event = threading.Event()

        # Parameters
        self.keyboard_frame = self.declare_parameter('keyboard_frame', 'keyboard_frame').get_parameter_value().string_value
        self.base_frame = self.declare_parameter('base_frame', 'arm_link').get_parameter_value().string_value
        self.ee_frame = self.declare_parameter('ee_frame', 'endeffector_kinematics').get_parameter_value().string_value
        self.actuator_frame = self.declare_parameter('actuator_frame', 'actuator').get_parameter_value().string_value

        # path planner arguments
        self.debug_target_tf = self.declare_parameter('debug_target', False).get_parameter_value().bool_value
        self.pp_speed = self.declare_parameter('speed', 0.5).get_parameter_value().double_value

        # Listen to /tf
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(self.tf_buffer, self)
        self.tf_timeout = self.declare_parameter('timeout', 2.0).get_parameter_value().double_value       # seconds to wait between each check
        self.tf_poll_rate = self.declare_parameter('poll_rate', 10.0).get_parameter_value().double_value  # check frequency in Hz

        # publish debug tf
        self.transform_broadcaster = TransformBroadcaster(self)

        self.sequence_pub = self.create_publisher(SequencerFeedback, SEQUENCER_TOPIC, 10)

        # Key localiser service client
        self.kblocaliser_client = self.create_client(KeyPosition, KEY_POSITION_SERVICE)
        while not self.kblocaliser_client.wait_for_service(timeout_sec=1.0):
            self.get_logger().info(f'{KEY_POSITION_SERVICE} service not available, waiting again...')

        # Path planner action client
        self.pplanner_client = ActionClient(self, ArmPlanPath, PATH_PLANNER_ACTION)
        self.action_executor = MultiThreadedExecutor()
        self.action_executor.add_node(self)

        # Pokey Thing action client
        self.pokey_client = ActionClient(self, EndEffector, POKEY_THING_ACTION)
        self.get_logger().info(f'Sequencer initalised!')

    def localise_key(self, key, stamp):
        request = KeyPosition.Request()
        request.key = key
        request.stamp = stamp
        response = self.kblocaliser_client.call(request)
        return response

    def compute_target_quaternion(self, keyboard_rotation):
        """Derive target wrist quaternion from actual keyboard orientation."""

        # First convert the keyboard quat to matrix form
        q = [keyboard_rotation.x, keyboard_rotation.y, keyboard_rotation.z, keyboard_rotation.w]
        kb_rmat = R.from_quat(q).as_matrix()

        # Z orientation is the opposite of the keyboard normal
        approach = -kb_rmat[:, 2]
        # X orientation is the same as the keyboard right direction
        right = kb_rmat[:, 0]
        # Remaining Y orientation is just the cross product of the two
        up = np.cross(approach, right)

        # Form the rotation matrix
        rmat = np.column_stack((right, up, approach))

        return R.from_matrix(rmat).as_quat()

    def pose_calc(self, key_to_base, ee_frame, actuator_frame, stamp) -> Pose | None:
        """ Given the following frames, calculates the pose to feed into the 
            path planner to move actuator of end effector offset from the key """
        act_to_ee = self.get_transform_from_frame(actuator_frame, ee_frame, stamp)
        if act_to_ee is None:
            return None
        ate_pose = Pose() 
        ate_pose.position = act_to_ee.transform.translation
        ate_pose.orientation = act_to_ee.transform.rotation
        ee_to_key = tf2_geometry_msgs.do_transform_pose(ate_pose, key_to_base)

        target_quaternion = self.compute_target_quaternion(key_to_base.transform.rotation)

        if self.debug_target_tf:
            tfs = TransformStamped()
            tfs.header.frame_id = self.base_frame
            tfs.child_frame_id = 'target_' + self.keyboard_frame
            tfs.header.stamp = stamp
            tfs.transform.translation = ee_to_key.position
            tfs.transform.rotation.x, tfs.transform.rotation.y, tfs.transform.rotation.z, tfs.transform.rotation.w = target_quaternion
            self.transform_broadcaster.sendTransform(tfs)

        return ee_to_key

    def call_path_planner(self, pose, speed):
        goal_msg = ArmPlanPath.Goal()
        goal_msg.pose = pose
        goal_msg.speed = speed

        if not self.pplanner_client.wait_for_server(timeout_sec=self.tf_timeout):
            self.get_logger().error('Path planner action server not available')
            return False
        future_response = self.pplanner_client.send_goal_async(goal_msg, feedback_callback=self.handle_pp_feedback)
        self.action_executor.spin_until_future_complete(future_response)
        response = future_response.result()
        if not response.accepted:
            self.get_logger().info('Path planner goal rejected')
            return False
            
        future_result = response.get_result_async()
        self.action_executor.spin_until_future_complete(future_result)
        return future_result.result

    def handle_pp_feedback(self, res):
        traversing_path = res.feedback.traversing_path
        path_generation_progress = res.feedback.path_generation_progress
        self.get_logger().info(f"Going? {traversing_path}, Progress: {path_generation_progress}")
    
    def do_poke(self, poke):
        goal_msg = EndEffector.Goal()
        goal_msg.poke = poke
        if not self.pokey_client.wait_for_server(timeout_sec=self.tf_timeout):
            self.get_logger().error('Pokey thing action server not available')
            return False
        future_response = self.pokey_client.send_goal_async(goal_msg, feedback_callback=self.handle_pk_feedback)
        self.action_executor.spin_until_future_complete(future_response)
        response = future_response.result()
        if not response.accepted:
            self.get_logger().info('Pokey Thing goal rejected')
            return False
        
        future_result = response.get_result_async()
        self.action_executor.spin_until_future_complete(future_result)
        result = future_result.result().result.end_poke
        return True

    
    def handle_pk_feedback(self, feedback_msg):
        feedback = feedback_msg.feedback
        self.get_logger().info(f'{feedback.current_poke} {feedback.is_forward}')

    def get_transform_from_frame(self, target_frame, source_frame, stamp=None) -> TransformStamped:
        self.get_logger().info(f'Getting transform of frame: {target_frame}')
        if stamp is None:
            stamp = self.get_clock().now() 
        start_time = self.get_clock().now()
        while self.get_clock().now() - start_time < rclpy.duration.Duration(seconds=self.tf_timeout):
            if self.tf_buffer.can_transform(target_frame, source_frame, stamp):
                transform = self.tf_buffer.lookup_transform(target_frame, source_frame, stamp)
                return transform
            time.sleep(1.0/self.tf_poll_rate)
        else:
            self.get_logger().warn(f'Transform of {target_frame} not available after waiting {self.tf_timeout} seconds')
        return None
    
    def return_to_start(self, start_pose):
        pp_return = self.call_path_planner(start_pose, self.pp_speed)
        if not pp_return:
            self.get_logger().info(f"Failed to return to start position")
            return False
        return True


    def start_sequencer(self, request, response):
        if self.thread and self.thread.is_alive():
            response.success = False
            response.message = "Sequencer already running."
            self.get_logger().info("Sequencer already running")
        else:
            response.success = True
            response.message = f"Sequencer successfully started with {request.sequence}"
            self.stop_event.clear()
            relocalise = request.relocalise
            self.thread = threading.Thread(target=self.execute_sequencer, args=[request.sequence, relocalise])
            self.thread.start()
        return response

    def stop_sequencer(self, request, response):
        if self.thread:
            self.stop_event.set()
            self.thread.join(timeout=1.0)
            self.thread = None
            self.get_logger().info("Sequencer stopped successfully.")
            response.success = True
            response.message = "Sequencer stopped successfully."
        else:
            self.get_logger().info("No sequencer running.")
            response.success = False
            response.message = "No sequencer running."
        return response

    def publish_error(self, seq_msg, error):
        self.get_logger().error(error)
        seq_msg.error = error
        seq_msg.current_key = ""
        self.sequence_pub.publish(seq_msg)
        self.stop_event.set()
        self.thread = None

    def execute_sequencer(self, key_sequence, relocalise) -> None:
        seq_msg = SequencerFeedback()
        seq_msg.sequence = key_sequence
        seq_msg.partial_sequence = []
        seq_msg.current_key = ""

        # Get position of EE in base link frame (Assumes operators have aligned keyboard with camera)
        ee_transform = self.get_transform_from_frame(self.base_frame, self.ee_frame)
        if ee_transform is None:
            self.publish_error(seq_msg, f"Failed getting {self.ee_frame} transform")
            return
        start_pose = Pose()
        start_pose.position = ee_transform.transform.translation
        start_pose.orientation = ee_transform.transform.rotation

        key_transforms = {}
        if not relocalise:
            # Get each key's transform
            for key in key_sequence:
                stamp = self.get_clock().now().to_msg()
                transform = self.localise_and_get_tf(key, stamp)
                if transform is None:
                    self.publish_error(seq_msg, f"Failed localising key '{key}'")
                    return
                key_transforms[key] = (transform, stamp)

        for key in key_sequence:
            if self.stop_event.is_set():
                self.get_logger().warn(f"Sequencer stopped at {key}")
                break

            self.get_logger().info(f'Performing sequence for key: {key}')
            seq_msg.current_key = key
            self.sequence_pub.publish(seq_msg)

            # Get the key's transform, or just retreive from dict
            if relocalise:
                stamp = self.get_clock().now().to_msg()
                key_transform = self.localise_and_get_tf(key, stamp)
                if key_transform is None:
                    self.publish_error(seq_msg, f"Failed localising key '{key}'")
                    return
            else:
                key_transform, stamp = key_transforms[key]

            # Start action to move to key via path planner
            pose = self.pose_calc(key_transform, self.ee_frame, self.actuator_frame, stamp)
            if pose is None:
                self.publish_error(seq_msg, f"Failed getting pose for '{key}'")
                return
            pp_result = self.call_path_planner(pose, self.pp_speed)
            if not pp_result:
                self.publish_error(seq_msg, f"Failed path planning for '{key}'")
                return
            self.get_logger().info(f"Path planner finished!")

            # Activate pokey thing
            if not self.do_poke(POKE_FORWARD) or not self.do_poke(POKE_BACKWARD):
                self.publish_error(seq_msg, f"Failed poking for '{key}'")
                return

            # Publish feedback to topic for GUI
            seq_msg.partial_sequence.append(key)
            self.get_logger().info(f'Completed: {seq_msg.partial_sequence}')

            # Move back to starting position after each key
            if relocalise:
                self.return_to_start(start_pose)

        # Move back to starting position after all keys
        if not relocalise:
            self.return_to_start(start_pose)

        seq_msg.current_key = ""
        self.sequence_pub.publish(seq_msg)
        self.get_logger().info(f'Sequencer Complete! {seq_msg.partial_sequence}')
        self.stop_event.set()
        self.thread = None

    def localise_and_get_tf(self, key, stamp):
        """Localise a key and return its transform, or None on failure."""
        # Ask keyboard localiser to publish the key transform to /tf
        key_result = self.localise_key(key, stamp)
        if not key_result.success:
            self.get_logger().warn(f"Failed localising {key}")
            return None
        
        # Retreive the key transform from /tf and store it
        key_frame = key + "_frame"
        key_transform = self.get_transform_from_frame(self.base_frame, key_frame, stamp)
        if key_transform is None:
            self.get_logger().warn(f"Failed getting transform for {key}")
        
        return key_transform


def main(args=None):
    rclpy.init(args=args)
    node = TypingSequencer()
    rclpy.spin(node)
    rclpy.shutdown()


if __name__ == '__main__':
    main()