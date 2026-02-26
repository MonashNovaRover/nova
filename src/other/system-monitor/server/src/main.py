import jcan
from flask import Flask
import json
import time

CAN_BUS = "vcan0" # can bus name to use
CAN_ID_PROBE = 0x500 # Probe request ID

STATIC_DIR = "../static" # where static files are
CONFIG_DIR = "../config" # where config files are

probe_responses = {
    # 0x400: {"firmware": 0, "errors": [], "responded_probe": 1}
}
probe_timestamps = {}
last_probe = -1 # last probe sent
can_ids_response = None # to be populated from ../static/can_ids.json

app = Flask(
    __name__,
    static_folder = STATIC_DIR
)
bus = jcan.Bus()

@app.get("/run_probe")
def send_probe():
    """
        Send probe to bus

        # CAN frame format:
         7                    0
        |                      |
        | uint8_t probe_number |
        |                      |
    """
    global last_probe
    last_probe += 1
    if last_probe > 255:
        last_probe = 255 # it's uint8_t on the microcontroller side so keep it below 255
    probe_frame = jcan.Frame(CAN_ID_PROBE, [last_probe])
    bus.send(probe_frame)

    # record time to timestamp. This is used when probe responses are checked
    now = time.time_ns()
    probe_timestamps[last_probe] = now
    print(f"Sent probe number {probe_frame}")
    return "sent"

def check_probe_responses():
    """
        Listen to probe responses from bus

        # CAN frame format:
         23                  16 15                       8 7                  0
        |                      |                          |                    |
        | uint8_t probe number | uint8_t firmware_version | uint8_t last_error | 
        |                      |                          |                    |
    """
    try:
        frame = bus.receive_with_timeout(0)
    except OSError:
        frame = None

    while frame is not None:
        if frame.id not in can_ids_response:
            continue 

        frame_id_hex = f"0x{frame.id:x}"

        if len(frame.data) != 3:
            print(f"Invalid probe response received from {frame_id_hex}")
            continue

        probe_number = frame.data[2]
        probe_time = probe_timestamps[probe_number]/10**6 # stored as ns but js expects ms
        firmware_version = frame.data[1]
        last_error = frame.data[0]

        print(f"Received probe response from {frame_id_hex}")
        probe_responses[frame_id_hex] = {
            "firmware": firmware_version,
            "last_error": last_error,
            "probe_time": probe_time
        }

        try:
            frame = bus.receive_with_timeout(0)
        except OSError:
            frame = None
    return 0

@app.get("/")
def index():
    with open(f"{STATIC_DIR}/index.html") as f:
        page = f.read()
    return page

@app.get("/get_probe_responses")
def app_get_probe_responses():
    check_probe_responses()
    return probe_responses

@app.get("/get_can_ids")
def app_get_can_ids():
    with open(f"{STATIC_DIR}/can_ids.json") as f:
        data = json.loads(f.read())
    data_idlower = {}
    for id in data:
        data_idlower[id.lower()] = data[id]
    return data_idlower

@app.get("/get_expected_can_ids")
def app_get_expected_can_ids():
    with open(f"{CONFIG_DIR}/expected_can_ids.json") as f:
        data = json.loads(f.read())
    for category in data["expected_id_categories"]:
        ids = data["expected_id_categories"][category] 
        for i in range(len(ids)):
            ids[i] = ids[i].lower()

    print(data)
    return data

if __name__ == "__main__":
    expected_ids_dict = app_get_expected_can_ids()
    can_ids_response = sum([[int(id, 16) for id in expected_ids_dict["expected_id_categories"][category]] for category in expected_ids_dict["expected_id_categories"]], [])
    bus.open(CAN_BUS)
    send_probe()
    app.run()

    
