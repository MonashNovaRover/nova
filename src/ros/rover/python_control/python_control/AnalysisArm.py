from control.ControllerNode import ControllerNode
from control.classes.cards.CMDCardController import CMDCardController
from control.classes.controls.OneAxisControl import OneAxisControl
from control.classes.sensors.TOFSensor import TOFSensor
from control.classes.limits.TOFLimit import TOFLimit
from control.classes.limits.LimitSwitchLimit import LimitSwitchLimit
from control.classes.sensors.LimitSwitchSensor import LimitSwitchSensor


class AnalysisArm(ControllerNode):


    CAN_BUS = "can1"
    CMD_ID = 0x00
    PLATFORM_MAX_PERCENT = 0.5

    TOF_FRAME_ID = 0x00
    LIMIT_SWITCH_FRAME_ID = 0x00
    LIMIT_SWITCH_COMMAND_ID = 0x00


    def __init__(self):
        super.__init__("AnalysisArm", self.CAN_BUS)

        tof_sensor = TOFSensor(
            can_bus=self.get_can_bus(),
            frame_id=self.TOF_FRAME_ID,
        )

        limit_switch_top = LimitSwitchSensor(
            can_bus=self.get_can_bus(),
            frame_id=self.LIMIT_SWITCH_FRAME_ID,
            command_id=self.LIMIT_SWITCH_COMMAND_ID,
        )

        platform_bottom_limit = TOFLimit(
            maximum=False,
            limit_value=10,
            tof_sensor=tof_sensor
        )

        platform_top_limit = LimitSwitchLimit(
            limit_switch=limit_switch_top
        )

        platform = OneAxisControl(
            max_percent=self.PLATFORM_MAX_PERCENT,
            pos_limit=platform_bottom_limit,
            neg_limit=platform_top_limit,
        )

        self.platform_controller = CMDCardController(
            can_bus=self.CAN_BUS,
            card_id=self.CMD_ID,
            control=platform,
        )

        

