import time
import jcan 
from rclpy.callback_groups import MutuallyExclusiveCallbackGroup
from .Controller import Controller 
from science_interfaces.srv import PowerCycle
from ..controller_manager.Interface import InterfaceCollection
from ..controller_manager.Contexts import Contexts

class PowerCycleController(Controller):
    def __init__(self, contexts: Contexts):
        super().__init__(contexts)
        self.cbg = MutuallyExclusiveCallbackGroup()
        self.srv = self.node.create_service(
            PowerCycle, 
            '/pc2/power_cycle_science', 
            self.power_cycle_callback,
            callback_group=self.cbg
        )
    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection) -> bool:
        return True

    def on_update(self, now: float, period: float):
        # We don't need a continuous control loop, we just wait for the service callback to fire.
        pass

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