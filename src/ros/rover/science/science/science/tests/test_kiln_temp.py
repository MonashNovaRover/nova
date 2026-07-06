#!/usr/bin/env python3

import rclpy
import json
import pytest


from science_interfaces.srv import ThermalCommand
SERVICE = '/science/thermal_command'



# Main function for setting up the ROS node    
def test_kiln_temp(setup_client_node):
    client = setup_client_node(
        services=[{'service': SERVICE, 'srv_type': ThermalCommand}], 
        topics=None
    )

    request = {'state': True, 'target': 0}
    future = client.send_request(SERVICE, request)
    rclpy.spin_until_future_complete(client, future)
    response = future.result()

    assert response.success

    # TODO: Add vcan monitoring
