import time

import rclpy
from rclpy.action import ActionServer
from rclpy.node import Node

from arm_interfaces.action import TypeSequence


class TypingSequencer(Node):

    def __init__(self):
        super().__init__('fibonacci_action_server')
        self.action_server = ActionServer(self, TypeSequence, 'type sequence', self.execute_callback)

    def execute_callback(self, action_handle):
        self.get_logger().info('Executing goal...')
        key_sequence = action_handle.request.sequence

        feedback_msg = TypeSequence.Feedback()
        feedback_msg.partial_sequence = []

        #TypeSequence.Feedback() for half way through
        #action_msg.succeed() when done good https://docs.ros2.org/foxy/api/rclpy/api/actions.html#rclpy.action.server.ServerGoalHandle.succeed
        #TypeSequence.Result() when done

        for key in key_sequence:
            # DO STUFF RELATED TO KEY
            feedback_msg.partial_sequence.append(key)
            self.get_logger().info('Feedback: {0}'.format(feedback_msg.partial_sequence))
            action_handle.publish_feedback(feedback_msg)

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