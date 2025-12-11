from . import output

"""
    Output that prints atrributes to the terminal; Not pretty at all. Quick fix to use for debugging so errors weren't hidden.
"""

class TerminalOut(output.Output):
    def __init__(self):
        pass

    def update(self, devices):
        for device in devices:
            print(f"Device: {device.name}")

            for attr in device.attrs:
                print(f"{attr.name}: {attr.value}")

    

from . import output

"""
    Output that prints atrributes to the terminal; to use for debugging.
"""

class TerminalOut(output.Output):
    def __init__(self):
        pass

    def update(self, devices):
        for device in devices:
            print(f"Device: {device.name}")

            for attr in device.attrs:
                print(f"{attr.name}: {attr.value}")

    
