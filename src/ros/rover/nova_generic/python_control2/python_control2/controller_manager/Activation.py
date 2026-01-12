from teleop_python_utils import Button

class Activation:
    """
    Class to represent whether a python control2 system is active or not

    Has a teleop button that when pressed activates the system and a
    pool of buttons that when pressed deactivates the system.
    """

    def __init__(self, active_button: Button, inactive_button_pool: list[Button], start_active: bool=False):
        """
        :param active_button: Button that activates.
        :param inactive_button_pool: Buttons that deactivate.
        :param start_active: Whether to start active or not, defaults to False.
        """
        self.active = start_active
        self.active_button = active_button
        self.inactive_button_pool = inactive_button_pool

        # Add button callbacks
        self.active_button.add_callback(self.activate)
        for but in inactive_button_pool:
            but.add_callback(self.deactivate)

    def activate(self):
        """ Activates the system """
        self.active = True

    def deactivate(self):
        """ Deactivates the system """
        self.active = False

    def is_active(self):
        return self.active

    def __bool__(self):
        return self.active
