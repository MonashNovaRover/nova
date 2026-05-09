#!/usr/bin/env python
'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

ROS action service which runs auto typing, 
    interfaces with the following:
- Keyboard localiser (auto_typing/keyboard_localiser.py)
- Path planner (nova_path_planner controller)
- Controller manager (ROS2 control SwitchController)
Used for auto typing task at URC
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: typing_sequencer
SERVICES: /type_sequence
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	auto_typing
AUTHOR(S):  Anthony Lew
CREATION:	9/05/2024
EDITED:     08/05/2026
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
from controller_manager_msgs.srv import ListControllers, SwitchController

import threading
import tf2_geometry_msgs

from arm_interfaces.msg import SequencerFeedback
from arm_interfaces.srv import KeyPosition, TypeSequence
from nova_interfaces.action import ArmPlanPath, EndEffector

TYPING_SEQUENCER_START = "/type_sequence/start"
TYPING_SEQUENCER_STOP = "/type_sequence/stop"
CONTROLLER_SWITCH_SERVICE = "/arm/controller_manager/switch_controller"
CONTROLLER_LIST_SERVICE = "/arm/controller_manager/list_controllers"
KEY_POSITION_SERVICE = "/arm/keyboard/pub_key_position"
PATH_PLANNER_ACTION = '/arm/plan_path'
POKEY_THING_ACTION = "/arm/poke"
SEQUENCER_TOPIC = "/arm/sequence"

# Controllers for auto typing mode
AUTO_TYPING_CONTROLLERS = ['nova_arm_position_controller', 'nova_path_planner']
# All teleop controllers that could conflict with auto typing
ALL_TELEOP_CONTROLLERS = [
    'nova_arm_velocity_controller',
    'nova_arm_position_controller',
    'nova_twistmapper',
    'nova_twistmapper_velocity',
    'nova_end_effector_velocity_controller',
]

POKE_FORWARD = 1.0
POKE_BACKWARD = 0.0

PATH_PLANNER_TIMEOUT = 10.0  # seconds to wait for path planner action server
POKEY_TIMEOUT = 10.0  # seconds to wait for pokey action server
CONTROLLER_SWITCH_TIMEOUT = 5.0  # seconds to wait for controller switch

DEFAULT_POSITION = [0.0, 0.0, 0.0]
DEFAULT_QUATERNION = [0.0, 0.0, 0.0, 1.0]

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
        self.move_to_start = self.declare_parameter('move_to_start', True).get_parameter_value().bool_value
        self.target_quaternion = self.declare_parameter('target_quaternion', DEFAULT_QUATERNION).get_parameter_value().double_array_value

        # Listen to /tf
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(self.tf_buffer, self)
        self.tf_timeout = self.declare_parameter('timeout', 2.0).get_parameter_value().double_value       # seconds to wait between each check
        self.tf_poll_rate = self.declare_parameter('poll_rate', 10.0).get_parameter_value().double_value  # check frequency in Hz

        # publish debug tf
        self.transform_broadcaster = TransformBroadcaster(self)

        self.sequence_pub = self.create_publisher(SequencerFeedback, SEQUENCER_TOPIC, 10)

        # Controller switcher service clients
        self.cswitcher_client = self.create_client(SwitchController, CONTROLLER_SWITCH_SERVICE)
        self.clist_client = self.create_client(ListControllers, CONTROLLER_LIST_SERVICE)
        self._previous_controllers = []

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

    def get_active_controllers(self):
        """ Query the controller manager for currently active controllers.
            Returns a list of active controller names, or None on failure."""
        if not self.clist_client.service_is_ready():
            self.get_logger().warn(f'{CONTROLLER_LIST_SERVICE} not available')
            return None

        future = self.clist_client.call_async(ListControllers.Request())
        self.action_executor.spin_until_future_complete(future, timeout_sec=CONTROLLER_SWITCH_TIMEOUT)
        if not future.done():
            self.get_logger().error('List controllers timed out')
            return None
        result = future.result()
        if result is None:
            return None
        return [c.name for c in result.controller if c.state == 'active']

    def switch_controllers(self, activate, deactivate):
        """ Switch ROS2 controllers via the controller manager service.
            Returns True on success, False on failure."""
        if not self.cswitcher_client.service_is_ready():
            self.get_logger().warn(f'{CONTROLLER_SWITCH_SERVICE} not available, skipping controller switch')
            return False

        request = SwitchController.Request()
        request.activate_controllers = activate
        request.deactivate_controllers = deactivate
        request.strictness = SwitchController.Request.BEST_EFFORT
        request.activate_asap = True

        future = self.cswitcher_client.call_async(request)
        self.action_executor.spin_until_future_complete(future, timeout_sec=CONTROLLER_SWITCH_TIMEOUT)
        if not future.done():
            self.get_logger().error('Controller switch timed out')
            return False
        result = future.result()
        if result is None or not result.ok:
            self.get_logger().error('Controller switch failed')
            return False
        self.get_logger().info(f'Switched controllers: activated={activate}, deactivated={deactivate}')
        return True

    def send_key_request(self, key, stamp):
        request = KeyPosition.Request()
        request.key = key
        request.stamp = stamp
        response = self.kblocaliser_client.call(request)
        return response

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

        if self.debug_target_tf:
            tfs = TransformStamped()
            tfs.header.frame_id = self.base_frame
            tfs.child_frame_id = 'target_' + self.keyboard_frame
            tfs.header.stamp = stamp
            tfs.transform.translation = ee_to_key.position
            #tfs.transform.rotation = ee_to_key.orientation
            tfs.transform.rotation.x, tfs.transform.rotation.y, tfs.transform.rotation.z, tfs.transform.rotation.w = self.target_quaternion
            self.transform_broadcaster.sendTransform(tfs)

        return ee_to_key

    def call_path_planner(self, pose, speed):
        goal_msg = ArmPlanPath.Goal()
        goal_msg.pose = pose
        goal_msg.speed = speed

        if not self.pplanner_client.wait_for_server(timeout_sec=PATH_PLANNER_TIMEOUT):
            self.get_logger().error(f'Path planner action server not available after {PATH_PLANNER_TIMEOUT}s. '
                                    'Is the nova_path_planner controller active?')
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

        if not self.pokey_client.wait_for_server(timeout_sec=POKEY_TIMEOUT):
            self.get_logger().error(f'Pokey action server not available after {POKEY_TIMEOUT}s')
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


    def start_sequencer(self, request, response):
        if self.thread and self.thread.is_alive():
            response.success = False
            response.message = "Sequencer already running."
            self.get_logger().info("Sequencer already running")
        else:
            response.success = True
            response.message = f"Sequencer successfully started with {request.sequence}"
            self.stop_event.clear()
            self.thread = threading.Thread(target=self.execute_sequencer, args=[request.sequence])
            self.thread.start()
        return response

    def stop_sequencer(self, request, response):
        if self.thread:
            self.stop_event.set()
            self.thread.join(timeout=1.0)
            self.thread = None
            # Restore previous controllers
            self.restore_previous_controllers()
            self.get_logger().info("Sequencer stopped successfully.")
            response.success = True
            response.message = "Sequencer stopped successfully."
        else:
            self.get_logger().info("No sequencer running.")
            response.success = False
            response.message = "No sequencer running."
        return response

    def restore_previous_controllers(self):
        """Restore the controllers that were active before auto typing started."""
        self.switch_controllers(
            activate=self._previous_controllers,
            deactivate=AUTO_TYPING_CONTROLLERS,
        )
        self._previous_controllers = []

    def execute_sequencer(self, key_sequence) -> None:
        # Save currently active controllers so we can restore them later
        active = self.get_active_controllers()
        self._previous_controllers = [c for c in (active or []) if c in ALL_TELEOP_CONTROLLERS]
        self.get_logger().info(f'Saving active teleop controllers: {self._previous_controllers}')

        # Switch to auto typing mode (deactivate all teleop, activate path planner)
        self.switch_controllers(
            activate=AUTO_TYPING_CONTROLLERS,
            deactivate=ALL_TELEOP_CONTROLLERS,
        )

        # Verify path planner is reachable before proceeding
        if not self.pplanner_client.wait_for_server(timeout_sec=PATH_PLANNER_TIMEOUT):
            self.get_logger().error(
                f'Path planner action server not available after {PATH_PLANNER_TIMEOUT}s. '
                'Cannot start typing sequence.')
            self.restore_previous_controllers()
            return

        # Get position of EE in base link frame (Assumes operators have aligned keyboard with camera)
        ee_transform = self.get_transform_from_frame(self.ee_frame, self.base_frame)
        if ee_transform is None:
            self.get_logger().info(f"Sequencer failed getting {self.ee_frame} transform")
            self.restore_previous_controllers()
            return
        start_pose = Pose() 
        start_pose.position = ee_transform.transform.translation
        start_pose.orientation = ee_transform.transform.rotation
        seq_msg = SequencerFeedback()
        seq_msg.sequence = key_sequence
        seq_msg.partial_sequence = []
        seq_msg.current_key = ""

        # Loop through the keys in the sequence
        for key in key_sequence:
            if self.stop_event.is_set():
                self.get_logger().warn(f"Sequencer stopped at {key}")
                return

            self.get_logger().info(f'Performing sequence for key: {key}')
            seq_msg.current_key = key
            self.sequence_pub.publish(seq_msg)
            # Get Key transform to be published on /tf by calling keyboard localiser
            stamp = self.get_clock().now().to_msg()
            key_result = self.send_key_request(key, stamp)
            
            # Get key transform
            key_frame = key + "_" + self.keyboard_frame
            self.get_logger().info(f'Get key: {key}')
            key_transform = self.get_transform_from_frame(self.base_frame, key_frame, stamp)
            if key_transform is None:
                self.get_logger().warn(f"Failed getting transform for {key}")
                break

            # Start action to move to key via path planner
            pose = self.pose_calc(key_transform, self.ee_frame, self.actuator_frame, stamp)
            if pose is None:
                self.get_logger().warn(f"Failed getting pose for {key}")
                break
            pp_result = self.call_path_planner(pose, self.pp_speed)
            if not pp_result:
                self.get_logger().warn(f"Path planner failed for {key}")
                break
            self.get_logger().info(f"Path planner finished!")

            # Activate pokey thing
            poke_out = self.do_poke(POKE_FORWARD)
            if not poke_out:
                self.get_logger().warn(f"Failed poking for {key}")
                break
            poke_in = self.do_poke(POKE_BACKWARD)
            if not poke_in:
                self.get_logger().warn(f"Failed retracting poke for {key}")
                break

            # Move back to starting position
            if self.move_to_start:
                self.call_path_planner(start_pose, self.pp_speed)

            # Publish feedback to topic for GUI
            seq_msg.partial_sequence.append(key)
            self.get_logger().info(f'Completed: {seq_msg.partial_sequence}')

        # Restore previous controllers
        self.restore_previous_controllers()

        seq_msg.current_key = ""
        self.sequence_pub.publish(seq_msg)
        self.get_logger().info(f'Sequencer Complete! {seq_msg.partial_sequence}')
        self.stop_event.set()
        self.thread = None



def main(args=None):
    rclpy.init(args=args)
    node = TypingSequencer()
    rclpy.spin(node)
    rclpy.shutdown()


if __name__ == '__main__':
    main()