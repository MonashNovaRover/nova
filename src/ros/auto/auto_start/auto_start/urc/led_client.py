#!/usr/bin/env python3
from nova_interfaces.srv import RGBInput

class LEDClient():
    def __init__(self, node):
        # Set parameters
        self.node = node
        self.started=False

        # Create service client for LED control
        self.led_client = self.node.create_client(RGBInput, '/set_RGBInput')
        self.node.get_logger().info('Waiting for /set_RGBInput server...')
        self.started = self.led_client.wait_for_service(timeout_sec=10.0)
        if not self.started:
            self.node.get_logger().error('Failed to find service /set_RGBInput! Cannot change LED color.')
            return
        self.node.get_logger().info('Succesfully found service /set_RGBInput.')

    def red(self):
        self.node.get_logger().info('Setting LEDs to red.')
        self.call((255, 0, 0), False)

    def blue(self):
        self.node.get_logger().info('Setting LEDs to blue.')
        self.call((0, 0, 255), False)

    def green(self):
        self.node.get_logger().info('Setting LEDs to green (flashing).')
        self.call((0, 255, 0), True)

    def call(self, rgb: tuple[int, int, int], flash: bool) -> bool:
        '''Calls the set_RGBInput service to change the LED color.'''
        request = RGBInput.Request()
        request.r = rgb[0]
        request.g = rgb[1]
        request.b = rgb[2]
        request.flash = flash
        future = self.led_client.call_async(request)
        future.add_done_callback(self.result)

    def result(self, future):
        '''Callback for the set_RGBInput service call.'''
        try:
            response = future.result()
            if response.success:
                self.node.get_logger().info('LED called and changed successfully.')
            else:
                self.node.get_logger().error('LED called but failed to change!')
        except Exception as e:
            self.node.get_logger().error(f'LED call failed: {e}')