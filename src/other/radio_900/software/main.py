import ctypes
import os
import time
import sys
from aim import move_to_zone, current_zone

# Load the shared C library
lib_path = os.path.abspath(os.path.join(os.path.dirname(__file__), '../firmware/build/libnetat_api.so'))
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

def rssi_to_bar(rssi, width=30):
    rssi = max(min(rssi, 20), -60)
    filled = int((rssi + 60) / 80 * width)  # From -60 to +20 is 80 dB range
    return '█' * filled + '-' * (width - filled)

def extract_rssi(response):
    try:
        for part in response.split():
            if "+RSSI:" in part:
                return int(part.split(":")[1])
    except Exception:
        pass
    return None

def poll_rssi(devices):
    num_devices = len(devices)
    print("\nRSSI Poll Table:\n")
    print("Idx  MAC Address         RSSI   Bar")
    print("-" * 50)
    print("\n" * num_devices, end='')  # Reserve lines for devices

    while True:
        print(f"\x1b[{num_devices}A", end='')  # Move cursor up to overwrite
        best_rssi = -999
        best_idx = None

        for i, mac in devices:
            if lib.netat_wrapper_set_target(i) != 0:
                line = f"[{i}] {mac_to_str(mac)} | ERR  | (failed)"
            else:
                buf = ctypes.create_string_buffer(10000)
                lib.netat_wrapper_send(b"AT+RSSI=?", buf, ctypes.sizeof(buf))
                response = buf.value.decode(errors="ignore").strip()
                rssi = extract_rssi(response)
                if rssi is not None:
                    bar = rssi_to_bar(rssi)
                    line = f"[{i+1}] {mac_to_str(mac)} | {rssi:>4} dBm | {bar}"
                    if rssi > best_rssi:
                        best_rssi = rssi
                        best_idx = i
                else:
                    line = f"[{i+1}] {mac_to_str(mac)} | ???  | (bad response)"
            print(line.ljust(50))
            sys.stdout.flush()

        # Move to the strongest signal's position
        if best_idx is not None:
            move_to_zone(best_idx)

        time.sleep(0.2)  # You can adjust this delay

def main():
    iface = "eth0"
    if lib.netat_wrapper_init(iface.encode()) != 0:
        print(f"Failed to initialize libnetat on interface: {iface}")
        return

    devices = get_device_list()
    if not devices:
        print("No devices found.")
        return

    try:
        poll_rssi(devices)
    except KeyboardInterrupt:
        print("\nStopped.")

if __name__ == "__main__":
    main()
