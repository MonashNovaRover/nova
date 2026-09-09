import ctypes
import os
from time import sleep
import readline
import atexit
import socket

# Enable command history across sessions
histfile = os.path.join(os.path.expanduser("~"), ".libnetat_history")

try:
    readline.read_history_file(histfile)
except FileNotFoundError:
    pass

atexit.register(readline.write_history_file, histfile)

# Load the shared C library
lib_path = os.path.abspath(os.path.join(os.path.dirname(__file__), '../../firmware/build/libnetat_api.so'))
lib = ctypes.CDLL(lib_path)

# Function signatures
lib.netat_wrapper_init.argtypes = [ctypes.c_char_p]
lib.netat_wrapper_init.restype = ctypes.c_int

lib.netat_wrapper_get_device_count.restype = ctypes.c_int
lib.netat_wrapper_get_device.argtypes = [ctypes.c_int, ctypes.POINTER(ctypes.c_ubyte)]

lib.netat_wrapper_set_target.argtypes = [ctypes.c_int]
lib.netat_wrapper_set_target.restype = ctypes.c_int

lib.netat_wrapper_send.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int]
lib.netat_wrapper_send.restype = ctypes.c_int

def mac_to_str(mac):
    return ':'.join(f'{b:02x}' for b in mac)

def get_device_list():
    count = lib.netat_wrapper_get_device_count()
    devices = []
    for i in range(count):
        mac = (ctypes.c_ubyte * 6)()
        lib.netat_wrapper_get_device(i, mac)
        devices.append((i, bytes(mac)))
    return devices

def select_device(devices):
    print("\nDiscovered devices:")
    for i, mac in devices:
        print(f"  [{i}] {mac_to_str(mac)}")
    try:
        selected = int(input("Select device index to communicate with: "))
        if lib.netat_wrapper_set_target(selected) != 0:
            print("Invalid device index.")
            return None
        return selected
    except ValueError:
        print("Invalid input.")
        return None

def run_sequence(commands):
    print("Running sequence...")
    for c in commands:
        print(f"> {c}")
        buf = ctypes.create_string_buffer(10000)
        lib.netat_wrapper_send(c.encode(), buf, ctypes.sizeof(buf))
        print(buf.value.decode(errors="ignore"))
        sleep(0.1)

def main():
    iface_choices = [item[1] for item in socket.if_nameindex() if item[1] != "lo"]
    if len(iface_choices) >= 1:
        print(f"Network interfaces:\n{'\n'.join([f'{i}: {iface}' for i, iface in enumerate(iface_choices)])}")
        ind = int(input(f"Index of interface to use? [{0}-{len(iface_choices)-1}]: "))
        iface = iface_choices[ind]
    else:
        print("No network interfaces found")
        iface = input("Name of interface to use? ")
    print("Refer to datasheet for commands: https://github.com/MonashNovaRover/antenna-tracking/blob/master/firmware/docs/AH.module.AT.command.development.guide_.ENGLISH.pdf")

    if lib.netat_wrapper_init(iface.encode()) != 0:
        print(f"Failed to initialize libnetat on interface: {iface}")
        return
    
    devices = get_device_list()
    if not devices:
        print("No devices found.")
        return

    current_device = select_device(devices)
    if current_device is None:
        return

    print("\nYou can now send AT commands (e.g., AT+RSSI=?).")
    print("Type 'nova sta' or 'nova ap' to setup with Nova defaults, 'switch' to select a different device or 'exit' to quit.\n")

    while True:
        cmd = input(">: ").strip()
        if cmd.lower() in ["exit", "quit"]:
            break
        elif cmd.lower() == "switch":
            current_device = select_device(devices)
            if current_device is None:
                print("Keeping previous target.")
            continue
        elif cmd.lower().startswith("nova "):
            if len(cmd.split()) <= 1:
                print("Give argument: 'ap' or 'sta'")
            elif cmd.split()[1] == "ap":
                commands = [
                    "AT+MODE=AP",
                    "AT+KEYMGMT=WPA-PSK",
                    "AT+PSK=1571410851082194661594662906776231843028669302874850927594572924",
                    "AT+PSK=1571410851082194661594662906776231843028669302874850927594572924",
                    "AT+BSS_BW=8",
                    "AT+FREQ_RANGE=9150,9280",
                    "AT+CHAN_LIST=9190",
                    "AT+TX_PWR_MAX=5",
                    "AT+SSID=nova900",
                    "AT+SYSDBG=WNB,0",
                    "AT+SYSDBG=LMAC,1",
                    "AT+PAIR=1"
                ]
                run_sequence(commands)
            elif cmd.split()[1] == "sta":
                commands = [
                    "AT+MODE=STA",
                    "AT+KEYMGMT=WPA-PSK",
                    "AT+PSK=1571410851082194661594662906776231843028669302874850927594572924",
                    "AT+PSK=1571410851082194661594662906776231843028669302874850927594572924",
                    "AT+BSS_BW=8",
                    "AT+FREQ_RANGE=9150,9280",
                    "AT+CHAN_LIST=9190",
                    "AT+TX_PWR_MAX=5",
                    "AT+SSID=nova900",
                    "AT+SYSDBG=WNB,0",
                    "AT+SYSDBG=LMAC,1",
                    "AT+PAIR=1"
                ]
                run_sequence(commands)
            else:
                print("Give argument: 'ap' or 'sta'")
        elif cmd.lower().startswith("at"):
            buf = ctypes.create_string_buffer(10000)
            lib.netat_wrapper_send(cmd.encode(), buf, ctypes.sizeof(buf))
            print(buf.value.decode(errors="ignore"))
        else:
            print("Not a valid AT command. Use 'AT+', 'switch', or 'exit'.")

if __name__ == "__main__":
    main()
