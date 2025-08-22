
class ControllerManager:

    def __init__(self, system_name: str):
        self.system_name = system_name
        self.contexts = Contexts()
        self.controllers: list[Controller] = []
        self.hardware_interfaces: list[HardwareInterface] = []
        pass

