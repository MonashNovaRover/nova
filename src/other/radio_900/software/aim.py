import RPi.GPIO as GPIO
import time

# --- Config ---
STEP_PIN = 27
DIR_PIN = 17
STEPS_PER_REV = 8160
ZONES = 6
STEPS_PER_POSITION = STEPS_PER_REV // ZONES  # = 1360
STEP_DELAY = 0.0002  # Fast = 200 µs per step

# --- Setup ---
GPIO.setmode(GPIO.BCM)
GPIO.setup(STEP_PIN, GPIO.OUT)
GPIO.setup(DIR_PIN, GPIO.OUT)

# --- State: track current zone (0 to 5) ---
current_zone = 0  # zone index: 0=pos1, 1=pos2, ..., 5=pos6


def move_to_zone(target_zone):
    """Move linearly between positions 1–6 without crossing 6→1 or 1→6."""
    global current_zone

    # if not (0 <= target_zone < ZONES):
    #     print("Invalid target zone")
    #     return

    diff = target_zone - current_zone

    if diff == 0:
        # print("Already at target.")
        return


    direction = GPIO.HIGH if diff > 0 else GPIO.LOW  # CW or CCW
    steps = abs(diff) * STEPS_PER_POSITION

    GPIO.output(DIR_PIN, direction)
    move_stepper(steps)

    current_zone = target_zone
    # print(f"Now at zone {current_zone + 1}")

def move_stepper(steps):
    for _ in range(steps):
        GPIO.output(STEP_PIN, GPIO.HIGH)
        time.sleep(STEP_DELAY)
        GPIO.output(STEP_PIN, GPIO.LOW)
        time.sleep(STEP_DELAY)

if __name__ == "__main__":
    try:
        print("Zones: 1–6. Cannot move directly between 1 and 6. Type 'q' to quit.")
        while True:
            val = input("Go to zone (1–6): ")
            if val.lower() == 'q':
                break
            if val.isdigit():
                zone = int(val)
                move_to_zone(zone)
                # if 1 <= zone <= 6:
                    # move_to_zone(zone)  # Convert 1–6 to 0–5
                # else:
                    # print("Invalid zone. Must be 1–6.")
            else:
                print("Invalid input.")
    finally:
        GPIO.cleanup()
