import time
import jcan 
from rclpy.callback_groups import ReentrantCallbackGroup
from .Controller import Controller 
from PowerCycle.srv import PowerCycle

class PowerCycleController(Controller):
    def __init__(self, node, name, contexts, **kwargs):
        super().__init__(node, name, contexts)
        self.node = node
        self.bus = contexts.get(jcan.Bus) 
        
        self.cb_group = ReentrantCallbackGroup()
        self.srv = self.node.create_service(
            PowerCycleScience, 
            '/pc2/power_cycle_science', 
            self.power_cycle_callback,
            callback_group=self.cb_group
        )

    def power_cycle_callback(self, request, response):
        self.node.get_logger().info(f'Power cycling science rails. Sleep: {request.sleep_duration}s')

        try:
            # send 001#01 and 001#10
            self.bus.send(jcan.Frame(0x001, [0x01]))
            self.bus.send(jcan.Frame(0x001, [0x10]))
            
            # sleep from GUI input
            time.sleep(request.sleep_duration)

            # send 002#01 and 002#10
            self.bus.send(jcan.Frame(0x002, [0x01]))
            self.bus.send(jcan.Frame(0x002, [0x10]))

            response.success = True
            response.message = "Power cycle complete."
        except Exception as e:
            response.success = False
            response.message = f"Failed to send CAN frames: {str(e)}"
            self.node.get_logger().error(response.message)

        return response