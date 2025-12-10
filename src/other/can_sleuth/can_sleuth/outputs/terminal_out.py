from . import output

"""
    Output that prints atrributes to the terminal; to use for debugging.
"""

def TerminalOut(output.Output):
    def __init__(self):
        pass

    def update(self, devices):
        for device in devices:
            print(f"Device: {device.name}")

            for attr in device.attrs:
                print(f"{attr.name}: {attr.value}")

    
