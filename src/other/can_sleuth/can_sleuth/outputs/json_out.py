'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Can Sleuth / Simulator

Json text output

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
EDITED BY: Orlando Chamberlain
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''

from . import output

import json

import sys

class JsonOut(output.Output):
    def __init__(self, file=None):
        if sys.stdout.isatty():
            self.indent = 4 # print pretty
        else:
            self.indent = None # print each json dict on one line


    def update(self, devices):
        data = {}
        for device in devices:
            if device.name not in data:
                data[device.name] = {}

            cls = type(device).__name__
            assert cls not in data[device.name], f"{cls} already in {data[device.name].keys()}"

            data[device.name][cls] = {attr.name: {
                    "value": attr.value,
                    # TODO: add units here etc if needed
                } for attr in device.attrs}

        print(json.dumps(data, indent=self.indent))
