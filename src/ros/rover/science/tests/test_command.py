#!/usr/bin/env python3

# Import all relevant ROS 2 packages
import rclpy, json
from rclpy.node import Node

# Import the service from the science package
from core.srv import ScienceCommand

# This is the main Node class that controls the functions
class ClientNode (Node):

    # The __init__ function needs to set up the Node.
    # It will create the node name and create the client with the relevant parameters
    def __init__(self):
        super().__init__('node_client')

        # Service Type, Service Name
        self.client = self.create_client(ScienceCommand, '/science/transmitter')
        
        # Waits until a service has been set up and running
        while not self.client.wait_for_service(timeout_sec=1.0):
            self.get_logger().info('Service not Available, Waiting again...')
        
        # Create the request data-frame
        self.req = ScienceCommand.Request()
    
    # Sends the request from the user input
    def send_request (self):
        # The science command
        command = {
            "target": "platform",
            "action": "scoops",
            "args": {
                "direction": "up",
                "id": 155
            }
        }
        self.req.command = json.dumps(command)

        # Call the science command
        self.future = self.client.call_async(self.req)

# Main function for setting up the ROS node    
def main (args = None):
    rclpy.init(args = args)
    client = ClientNode()
    client.send_request()
    
    # Loop until ROS is not okay
    while rclpy.ok():
        rclpy.spin_once(client)
        
        # If the client has completed processing
        if client.future.done():
            # Attempt to get a response
            try:
                response = client.future.result()
            # Handle the error if missing data
            except Exception as e:
                client.get_logger().info("Service call failed %r" % (e,))
            # If success
            else:
                client.get_logger().info(
                    "Response: %s" % response.success)
            break
    
    # Shutdown ROS otherwise
    client.destroy_node()
    rclpy.shutdown()

# This code is called when 'python3' is used to run the script
if __name__ == '__main__':
    main()
