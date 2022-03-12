import json

# Standard CAN ID
CAN_ID = 0


# New looking for CMD
# WILL NEED TO UPDATE THIS
TARGET_USE_CMD = {
    "payload": True,
    "hydraprobe": False,
    "kiln": False
}

ACTION_CMD_ID = {
    "scoop": "09",
    "linear_actuator": "08",
}


# Old Target Dictionary

TARGET_DICT =  {
    "payload": "0",
    "hydraprobe": "000",
    "kiln": "001"
}

ACTION_DICT = {
    "hydraprobe_limits": "01",
    "hydraprobe": "02",
    "hydraprobe_top": "03",
    "hydraprobe_bottom": "04",
    "kiln_lid": "01",
    "kiln_mixer": "02",
    "kiln_heater": "03",
}

ARGUMENT_DICT = {
    "forward": "00",
    "reverse": "01",
    "up": "00",
    "down": "01",
    "true": "01",
    "false": "00",
}

'''
Function description
'''
def parse_command(command_dict):
    """
    Command dict requirements.
    * must fill arguments from left to right
    * if not in args dict must be speed or value of range [0, 255] 
    """

    target = command_dict["target"]
    action = command_dict["action"]
    args = command_dict["args"]

    print(target, action, args)
    target_code = TARGET_DICT[target]
    action_code = ACTION_DICT[action]
    argument_codes = []
    for argument in args: 
        if argument == "scoop_id":
            argument_codes.append(args[argument])
        else:
            try:
                arg = ARGUMENT_DICT[args[argument]]
            except KeyError:  # if not in dict, assume value is just a number of two hex digits
                hex = format(int(args[argument]), 'x')
                arg = hex.zfill(2)
            argument_codes.append(arg)

    code = action_code + "".join(argument_codes)
    code = code.ljust(8, "0")
    return (target_code, code)


def parse(data):
    json_data = json.load(data)

    # Check if command is CMD or PICS
    # If CMD
    if TARGET_USE_CMD[json_data["target"]]:
        speed = json_data["args"].get("speed", False)
        # if given a speed
        if json_data["args"]["speed"]:
            arg = "3"
            command = speed
        elif json_data["args"]["direction"] in ("forward", "down"):
            arg = "1"
            command = "00"
        else:
            command = "00"
            arg = "2"

        # Update the CAN ID
        new_id = ACTION_CMD_ID[json_data["action"]] + arg
        arbitration_id = int(new_id)

    # If PICS
    else:
        # Parses the command
        id, command = parse_command(json_data)
        print("Executing Command: %s" % command)

        # Update the CAN ID
        arbitration_id = int(id, 16)

    print('arb id: ' + str(arbitration_id))

    print("command: " + str(command))

    # Execute the CAN command
    # response.success = self.can.transmit(bytearray.fromhex(command))

    # Return the response
    return


data = open('data.json')
parse(data)
