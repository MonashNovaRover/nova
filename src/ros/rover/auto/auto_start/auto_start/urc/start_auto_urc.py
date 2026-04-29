#!/usr/bin/env python3
'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Node that loads waypoints from a file and 
sends it to the /urc_2025_navigator action
server. It continuously checks the status of the 
action server to monitor if it has been
aborted. If the action server has aborted, it will 
reload the waypoints to restart navigation.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: StartAuto
TOPICS:
    - subscriber: /blackboard                   [std_msgs/msg/String]
    - publisher: /auto/status             [nova_interfaces/msg/Status]
SERVICES:
    - client: /fromLL                           [robot_localization/srv/FromLL]
    - client: /set_RGBInput                     [nova_interfaces/srv/RGBInput]
    - service: /autonomous/cartographer_command [nova_interfaces/srv/CartographerCommand]
ACTIONS: 
  - client: /urc_2025_navigator                 [URCThroughPoses]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	nova_utils
AUTHOR(S):	Tarik Thomas, Terry Tian, 
            Victor Bartlinski
CREATION:	27/05/2025
EDITED:		28/04/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
import rclpy
from rclpy.node import Node
from rclpy.qos import QoSPresetProfiles
from rclpy.executors import MultiThreadedExecutor
from std_msgs.msg import String
from geometry_msgs.msg import PoseStamped
from nova_interfaces.msg import Status
from action_msgs.msg import GoalStatus
import yaml,os,enum
from auto_start.urc.cartographer_client import CartographerClient
from auto_start.urc.fromll_client import FromLLClient
from auto_start.urc.led_client import LEDClient
from auto_start.urc.navigator_client import NavigatorClient

class State(enum.Enum):
    WAITING_FOR_CARTOGRAPHER=0,
    CONVERTING_LLS_TO_POSES=1,
    STARTING_NAVIGATION=2,
    MONITOR_NAVIGATION=3,

class StartAuto(Node):
    def __init__(self):
        super().__init__('start_auto')
        # Declare and get parameters
        self.started=False
        self.filepath=self.declare_parameter(name='filepath', 
            value=os.path.expanduser('~/.ros/waypoints.yaml'), 
        ).value
        self.status_topic=self.declare_parameter(name='status_topic',
            value='/auto/status', 
        ).value
        self.blackboard={}
        
        self.cartographer_client = CartographerClient(self)

        self.fromll_client = FromLLClient(self)
        if not self.fromll_client.started:
            return

        self.led_client = LEDClient(self)
        # if not self.led_client.started:
        #     return

        self.navigator_client = NavigatorClient(self)
        if not self.navigator_client.started:
            return

        # Save waypoints
        self.blackboard_subscriber = self.create_subscription(String, '/blackboard', self.blackboard_callback, QoSPresetProfiles.SENSOR_DATA.value)
        self.get_logger().info('Subscriber /blackboard created.')

        # Create publisher for navigation status
        self.status = Status.IDLE
        self.status_publisher = self.create_publisher(Status, self.status_topic, 1)
        self.status_timer = self.create_timer(1, self.publish_status)
        self.get_logger().info(f'Publisher {self.status_topic} created.')

        # Create a timer
        self.state = State.WAITING_FOR_CARTOGRAPHER
        self.state_timer = self.create_timer(0.5, self.tick)

        self.started = True

    def tick(self):
        match self.state:

            case State.WAITING_FOR_CARTOGRAPHER:
                self.get_logger().info('State: WAITING_FOR_CARTOGRAPHER.')
                if self.cartographer_client.received_goals():
                    self.fromll_client.lls_to_poses(self.cartographer_client.goals)
                    self.cartographer_client.reset()
                    self.state = State.CONVERTING_LLS_TO_POSES
            
            case State.CONVERTING_LLS_TO_POSES:
                self.get_logger().info('State: CONVERTING_LLS_TO_POSES.')
                if self.fromll_client.waiting():
                    return
                
                if self.fromll_client.has_next():
                    self.fromll_client.call()
                elif self.fromll_client.converted_goals():
                    self.state = State.STARTING_NAVIGATION

            case State.STARTING_NAVIGATION:
                self.get_logger().info('State: STARTING_NAVIGATION.')
                # Save waypoints to avoid race condition where the BT fails before waypoints are published to /blackboard
                self.save_waypoints(self.poses_to_yaml(self.fromll_client.poses))
                self.led_client.red()
                self.navigator_client.start(self.cartographer_client.goal_type, self.fromll_client.poses, self.cartographer_client.search_radius)
                self.state = State.MONITOR_NAVIGATION

            case State.MONITOR_NAVIGATION:
                self.get_logger().info('State: MONITOR_NAVIGATION.')
                if self.navigator_client.finished():
                    if self.navigator_client.status == GoalStatus.STATUS_SUCCEEDED:
                        self.led_client.green()
                        self.publish_status(Status.ARRIVED_SUCCESSFULLY)
                        self.get_logger().info(f'Navigation succeeded: {self.navigator_client.status}')
                    elif self.navigator_client.status == GoalStatus.STATUS_CANCELED:
                        self.get_logger().warn(f'Navigation cancelled: {self.navigator_client.status}')
                    elif self.navigator_client.status == GoalStatus.STATUS_ABORTED:
                        self.get_logger().error(f'Navigation aborted! {self.navigator_client.status}')
                    else:
                        self.get_logger().error(f'Navigation ended with unknown status: {self.navigator_client.status}')
                    self.state = State.WAITING_FOR_CARTOGRAPHER
                    return

                if not self.navigator_client.goal_handle:
                    self.get_logger().warn('No active goal handle! Skipping navigation status check.')
                    return

                if self.navigator_client.goal_handle.status == GoalStatus.STATUS_ABORTED:
                    self.get_logger().error('Navigation aborted detected by timer callback!')
                    self.get_logger().info('Sending waypoints to restart navigation')
                    self.navigator_client.start(self.cartographer_client.goal_type, self.load_waypoints(), self.cartographer_client.search_radius)

    def poses_to_yaml(self, poses):
        waypoints = {}
        for i, pose in enumerate(poses, start=1):
            waypoints[f'waypoint_{i}'] = {
                'pose': [
                    pose.pose.position.x,
                    pose.pose.position.y,
                    pose.pose.position.z,
                ],
                'orientation': [
                    pose.pose.orientation.x,
                    pose.pose.orientation.y,
                    pose.pose.orientation.z,
                    pose.pose.orientation.w,
                ]
            }
        return {'waypoints': waypoints}
    
    def save_waypoints(self, waypoints):
        ''' Saves the extracted waypoints to a YAML file. '''
        with open(self.filepath, 'w') as f:
            yaml.dump({'waypoints': waypoints}, f, indent=2)
        self.get_logger().info(f'Saved waypoints to: {self.filepath}')

    def load_waypoints(self):
        '''Loads waypoints from YAML file and converts them into PoseStamped messages.'''
        if not os.path.exists(self.filepath):
            self.get_logger().error(f'Failed to load waypoints: {self.filepath} does not exist!')
            return None

        with open(self.filepath, 'r') as f:
            data = yaml.load(f, yaml.Loader)

        waypoints_data = data.get('waypoints', [])
        if not waypoints_data:
            self.get_logger().warn('Waypoints not found in YAML file.')
            return None
        waypoints = []
        for idx, wp in enumerate(waypoints_data):
            goal = PoseStamped()
            goal.header.frame_id = 'map'
            goal.header.stamp = self.get_clock().now().to_msg()
            goal.pose.position.x = waypoints_data[wp]['pose'][0]
            goal.pose.position.y = waypoints_data[wp]['pose'][1]
            goal.pose.position.z = waypoints_data[wp]['pose'][2]
            goal.pose.orientation.x = waypoints_data[wp]['orientation'][0]
            goal.pose.orientation.y = waypoints_data[wp]['orientation'][1]
            goal.pose.orientation.z = waypoints_data[wp]['orientation'][2]
            goal.pose.orientation.w = waypoints_data[wp]['orientation'][3]
            waypoints.append(goal)
            self.get_logger().info(f'Loaded Waypoint {idx+1}: ({goal.pose.position.x:.2f}, {goal.pose.position.y:.2f})')
        return waypoints

    def blackboard_callback(self, msg):
        '''
        Saves the blackboard data to a dictionary and extracts waypoints, saving them.
        Also publishes the status seen in the blackboard to the status topic.
        '''
        for entry in msg.data.strip().split('\n'):
            if ': ' not in entry:
                continue  # skip malformed or empty lines
            key, value = entry.split(': ', 1)
            self.blackboard[key] = value

        # for entry in msg.data.strip().split('\n'):
        #     key, value = entry.split(': ', 1)
        #     self.blackboard[key] = value

        waypoints = {}
        try:
            goals = self.blackboard["goals"].split('\n')[0].split('(')[1:]
            for i in range(len(goals)):
                coords = goals[i].split(')')[0].split(', ')
                pos_x = float(coords[0])
                pos_y = float(coords[1])
                pos_z = float(coords[2])
                rot_x = float(coords[3])
                rot_y = float(coords[4])
                rot_z = float(coords[5])
                rot_w = float(coords[6])
                waypoints[f'waypoint{i}'] = {
                    'pose': [pos_x, pos_y, pos_z],
                    'orientation': [rot_x, rot_y, rot_z, rot_w]
                }
        except Exception as e:
            self.get_logger().error(f'Error extracting waypoints: {e}')
            return None

        if waypoints:
            self.save_waypoints(waypoints)

        self.status = int(self.blackboard.get('status', Status.IDLE))  # Default to 0 if not found

    def publish_status(self, status=None) -> None:
        '''Publishes the current navigation status to the status topic.'''
        msg = Status()
        if status is None:
            msg.status = self.status
        else:
            msg.status = status
        self.status_publisher.publish(msg)

def main(args=None):
    '''Main function to start the ROS2 node.'''
    rclpy.init(args=args)
    node = StartAuto()
    if not node.started:
        node.destroy_node()
        rclpy.shutdown()
        return
    executor = MultiThreadedExecutor()
    executor.add_node(node)
    try:
        executor.spin()
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()
