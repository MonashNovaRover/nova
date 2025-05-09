import time

import rclpy
from rclpy.action import ActionServer, ActionClient
from rclpy.node import Node
from tf2_ros import Buffer, TransformListener

from arm_interfaces.action import TypeSequence, PathTo
from arm_interfaces.srv import KeyPosition

TYPING_SEQUENCER_ACTION = "/type_sequence"
KEY_POSITION_SERVICE = "/pub_key_position"
PATH_PLANNER_ACTION = "/move_to_pos"

class TypingSequencer(Node):

    def __init__(self):
        super().__init__('typing_sequencer')
        self.action_server = ActionServer(self, TypeSequence, TYPING_SEQUENCER_ACTION, self.execute_sequencer)

        # Parameters
        self.keyboard_frame = self.declare_parameter('keyboard_frame', 'keyboard_frame').get_parameter_value().string_value
        self.base_frame = self.declare_parameter('base_frame', 'base_link').get_parameter_value().string_value
        self.ee_frame = self.declare_parameter('ee_frame', 'eebase').get_parameter_value().string_value

        # Listen to /tf
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(self.tf_buffer, self)
        self.tf_timeout = self.declare_parameter('timeout', 2.0).get_parameter_value().double_value     # seconds to wait between each check
        self.tf_poll_rate = self.declare_parameter('timeout', 10.0).get_parameter_value().double_value  # check frequency in Hz

        # Key localiser service client
        self.kblocaliser_client = self.create_client(KeyPosition, KEY_POSITION_SERVICE)
        while not self.kb_localiser_client.wait_for_service(timeout_sec=1.0):
            self.get_logger().info(f'{KEY_POSITION_SERVICE} service not available, waiting again...')

        # Path planner action client
        self.pplanner_client = ActionClient(self, PathTo, PATH_PLANNER_ACTION)

    def send_key_request(self, key, stamp):
        request = KeyPosition.request()
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
        return result.done

    
    def handle_pp_feedback(self, feedback_msg):
        feedback = feedback_msg.feedback
        self.get_logger().info('Recieved feedback: {0}'.format(feedback.status))


    def execute_sequencer(self, action_handle):
        self.get_logger().info('Beginning sequencer...')
        key_sequence = action_handle.request.sequence

        feedback_msg = TypeSequence.Feedback()
        feedback_msg.partial_sequence = []

        # Get position of EE in base link frame (Assumes operators have aligned keyboard with camera)
        ee_transform = self.tf_buffer.lookup_transform(self.ee_frame, self.base_frame, stamp)

        # Call controller switcher and switch to "Auto typing mode" (IK only mode)
        for key in key_sequence:
            # Get Key transform to be published on /tf by calling keyboard localiser
            stamp = self.get_clock().now().to_msg()
            self.send_key_request(key, stamp)
            
            # Wait for transform
            key_frame = key + "_" + self.keyboard_frame
            start_time = self.get_clock().now()
            while self.get_clock().now() - start_time < rclpy.duration.Duration(seconds=self.timeout):
                if self.tf_buffer.can_transform(key_frame, self.ee_frame, stamp):
                    transform = self.tf_buffer.lookup_transform(key_frame, self.ee_frame, stamp)
                    break
                time.sleep(1.0/self.poll_rate)
            else:          
                self.get_logger().warn('Transform not available after waiting {:.1f} seconds'.format(self.timeout))
                
            # Start action to move to key via path planner
            self.path_to_tf
            # Activate pokey thing
            # Move back to starting position
            feedback_msg.partial_sequence.append(key)
            
            self.get_logger().info('Feedback: {0}'.format(feedback_msg.partial_sequence))
            action_handle.publish_feedback(feedback_msg)

        # Call controller switcher and switch back to manual mode
        goal_handle.succeed()

        result = Fibonacci.Result()
        result.sequence = feedback_msg.partial_sequence
        return result


def main(args=None):
    rclpy.init(args=args)
    node = TypingSequencer()
    rclpy.spin(node)


if __name__ == '__main__':
    main()