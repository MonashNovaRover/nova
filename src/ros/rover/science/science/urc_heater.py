#!/usr/bin/env python3
import rclpy
from std_srvs.srv import SetBool

from python_control.ControllerNode import ControllerNode
from python_control.controllers.ToggleController import ToggleController
from python_control.controls.ToggleControl import ToggleControl


class URCHeater(ControllerNode):
    # CAN BUS NAME
    # The name of the CAN bus to use
    CAN_BUS = "can1"

    # SENDING CARD IDS
    # Add any CONTROL FRAME / CARD IDS here
    HEATER_CONTROL_SEND_FRAME = 0x0B0
    HEATER_READ_SEND_FRAME = 0x4B3

    # ROS2 SERVICES
    HEATER_SERVICE = "/science/heater"

    # CONTROL NAMES
    # Add any CONTROL names here
    HEATER_CONTROL = "heater"

    # SENDING COMMAND IDS
    # Add any CONTROL command ids here
    HEATER_CONTROL_ID = 0x05
    HEATER_SEND_ON = 0xFF
    HEATER_SEND_OFF = 0x00

    def __init__(self):
        super().__init__(name="urc_heater", can_bus=self.CAN_BUS)
        logger = self.get_logger()

        self.heater_control = ToggleControl(logger=logger, on=False)
        self.heater_controller = ToggleController(
            logger=logger,
            bus=self.bus,
            frame_id=self.HEATER_CONTROL_SEND_FRAME,
            control_id=self.HEATER_CONTROL_ID,
            toggle_command_on=self.HEATER_SEND_ON,
            # toggle_command_off=self.HEATER_SEND_OFF, # Relies on heartbeat instead
            control=self.heater_control,
        )
        self.add_controller(self.HEATER_CONTROL, self.heater_controller)

        self.start_can()

        self.create_service(SetBool, self.HEATER_SERVICE, self.toggle_callback)

    def toggle_callback(
        self,
        request: SetBool.Request,
        response: SetBool.Response,
    ) -> SetBool.Response:
        if request.data:
            self.heater_control.start()
        else:
            self.heater_control.stop()
        response.success = True
        return response


def main():
    rclpy.init()
    urc_heater = URCHeater()
    rclpy.spin(urc_heater)
    rclpy.shutdown()


if __name__ == "__main__":
    main()
