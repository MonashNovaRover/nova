import jcan
from python_control.ControllerNode import ControllerNode
from python_control.classes.cards.CMDCardController import CMDCardController
from python_control.classes.controls.OneAxisControl import OneAxisControl
from python_control.classes.sensors.IntegerSensor import IntegerSensor
from python_control.classes.limits.IntegerLimit import IntegerLimit
from python_control.classes.limits.LimitSwitchLimit import LimitSwitchLimit
from python_control.classes.sensors.LimitSwitchSensor import LimitSwitchSensor


class AnalysisArm(ControllerNode):

    CAN_BUS = "can1"
    CMD_ID = 0x10
    PLATFORM_MAX_PERCENT = 0.5

    TOF_FRAME_ID = 0x01
    LIMIT_SWITCH_FRAME_ID = 0x02
    LIMIT_SWITCH_COMMAND_ID = 0x03


    def __init__(self):
        super.__init__("AnalysisArm", self.CAN_BUS)

        ## Add CAN ID Filters
        self.bus.set_id_filter([self.TOF_FRAME_ID, self.LIMIT_SWITCH_FRAME_ID])

        ## Create sensors
        tof_sensor = IntegerSensor(
            bus=self.bus,
            frame_id=self.TOF_FRAME_ID,
        )

        limit_switch_top = LimitSwitchSensor(
            bus=self.bus,
            frame_id=self.LIMIT_SWITCH_FRAME_ID,
            command_id=self.LIMIT_SWITCH_COMMAND_ID,
        )

        # Create limits
        platform_bottom_limit = IntegerLimit(
            bus=self.bus,
            maximum=False,
            limit_value=10,
            integer_sensor=tof_sensor
        )

        platform_top_limit = LimitSwitchLimit(
            bus=self.bus,
            limit_switch=limit_switch_top
        )

        ## Create controls
        platform = OneAxisControl(
            max_percent=self.PLATFORM_MAX_PERCENT,
            pos_limit=platform_bottom_limit,
            neg_limit=platform_top_limit,
        )

        ## Create controllers
        self.platform_controller = CMDCardController(
            bus=self.bus,
            card_id=self.CMD_ID,
            control=platform,
        )

        ## Add the controllers to the node's of controllers
        self.add_controller("platform", self.platform_controller)

        ## Start the CAN bus
        self.start_can()

        

