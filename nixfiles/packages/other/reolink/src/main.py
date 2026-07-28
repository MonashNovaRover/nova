#!/usr/bin/env python3
"""
Reolink PTZ Camera Control Script
Adapted from work by Roger Hardiman <opensource@rjh.org.uk>
https://github.com/agsh/onvif/blob/master/examples/example3.js
"""

import os
import sys
import time
import threading
from pathlib import Path
from onvif import ONVIFCamera
from pynput import keyboard

# Configuration
HOSTNAME = '10.0.1.100'
PORT = 80
USERNAME = 'admin'
PASSWORD_FILE = Path.home() / 'nova' / 'nixfiles' / 'secrets' / 'reolink-password.txt'
STOP_DELAY_MS = 200

# Global state
auto_mode = False
auto_timer = None
auto_count = 0
stop_timer = None
ignore_keypress = False
velocity = {'x': 0, 'y': 0, 'zoom': 0}
cam = None
ptz_service = None

# Velocity presets
left = {'x': -1, 'y': 0, 'zoom': 0}
right = {'x': 1, 'y': 0, 'zoom': 0}
up = {'x': 0, 'y': 1, 'zoom': 0}
down = {'x': 0, 'y': -1, 'zoom': 0}

# Auto-pan sequence
auto_sequence = [
    {'vel': left, 'time': 600},
    {'vel': left, 'time': 600},
    {'vel': left, 'time': 600},
    {'vel': left, 'time': 600},
    {'vel': left, 'time': 600},
    {'vel': left, 'time': 600},
    {'vel': left, 'time': 600},
    {'vel': left, 'time': 600},
    {'vel': left, 'time': 600},
    {'vel': left, 'time': 600},
    {'vel': left, 'time': 600},
    {'vel': left, 'time': 600},
    {'vel': left, 'time': 600},
    {'vel': left, 'time': 600},
    {'vel': left, 'time': 600},
    {'vel': left, 'time': 600},
    {'vel': left, 'time': 600},
    {'vel': left, 'time': 600},
    {'vel': left, 'time': 600},
    {'vel': left, 'time': 600},
    {'vel': up, 'time': 800},
    {'vel': right, 'time': 600},
    {'vel': right, 'time': 600},
    {'vel': right, 'time': 600},
    {'vel': right, 'time': 600},
    {'vel': right, 'time': 600},
    {'vel': right, 'time': 600},
    {'vel': right, 'time': 600},
    {'vel': right, 'time': 600},
    {'vel': right, 'time': 600},
    {'vel': right, 'time': 600},
    {'vel': right, 'time': 600},
    {'vel': right, 'time': 600},
    {'vel': right, 'time': 600},
    {'vel': right, 'time': 600},
    {'vel': right, 'time': 600},
    {'vel': right, 'time': 600},
    {'vel': right, 'time': 600},
    {'vel': right, 'time': 600},
    {'vel': right, 'time': 600},
    {'vel': right, 'time': 600},
    {'vel': down, 'time': 1000},
]


def read_password():
    """Read password from file."""
    try:
        return PASSWORD_FILE.read_text().strip()
    except Exception as e:
        print(f"Error reading password file: {e}")
        sys.exit(1)


def clear_stop():
    """Clear any pending stop timer."""
    global stop_timer
    if stop_timer:
        stop_timer.cancel()
        stop_timer = None


def schedule_stop():
    """Schedule a stop command after STOP_DELAY_MS."""
    global stop_timer
    clear_stop()
    stop_timer = threading.Timer(STOP_DELAY_MS / 1000.0, stop)
    stop_timer.start()


def stop():
    """Send stop command to camera."""
    global velocity, ptz_service
    try:
        request = ptz_service.create_type('Stop')
        request.ProfileToken = profile_token
        request.PanTilt = True
        request.Zoom = True
        ptz_service.Stop(request)
        velocity = {'x': 0, 'y': 0, 'zoom': 0}
    except Exception as e:
        print(f"Error stopping: {e}")


def move(new_velocity):
    """Move camera with given velocity."""
    global velocity, ignore_keypress, ptz_service

    # Check if we need to update the currently running move command
    if (new_velocity['x'] == velocity['x'] and
        new_velocity['y'] == velocity['y'] and
        not new_velocity['zoom']):  # Zoom works better if sent repeatedly
        # Reschedule timeout
        schedule_stop()
        return

    velocity = new_velocity.copy()

    # Pause keyboard processing
    ignore_keypress = True

    # Clear any pending stop commands
    clear_stop()

    try:
        # Move the camera
        request = ptz_service.create_type('ContinuousMove')
        request.ProfileToken = profile_token
        request.Velocity = {
            'PanTilt': {'x': velocity['x'], 'y': velocity['y']},
            'Zoom': {'x': velocity['zoom']}
        }
        ptz_service.ContinuousMove(request)
        print(f"move command sent  {velocity}")
        schedule_stop()
    except Exception as e:
        print(f"Error moving: {e}")
    finally:
        # Resume keyboard processing
        ignore_keypress = False


def schedule_auto_timer(time_ms):
    """Schedule the next auto-pan movement."""
    global auto_timer, auto_mode
    if auto_timer:
        auto_timer.cancel()
        auto_timer = None
    if auto_mode:
        auto_timer = threading.Timer(time_ms / 1000.0, auto_cb)
        auto_timer.start()


def auto_cb():
    """Auto-pan callback - execute next movement in sequence."""
    global auto_count, ptz_service

    try:
        vel = auto_sequence[auto_count]['vel']
        request = ptz_service.create_type('ContinuousMove')
        request.ProfileToken = profile_token
        request.Velocity = {
            'PanTilt': {'x': vel['x'], 'y': vel['y']},
            'Zoom': {'x': vel['zoom']}
        }
        ptz_service.ContinuousMove(request)

        schedule_auto_timer(auto_sequence[auto_count]['time'])
        auto_count = (auto_count + 1) % len(auto_sequence)
    except Exception as e:
        print(f"Error in auto mode: {e}")


def on_press(key):
    """Handle key press events."""
    global auto_mode, auto_count, ignore_keypress

    # Exit on 'q' or Ctrl+C
    if key == keyboard.KeyCode.from_char('q'):
        auto_mode = False
        schedule_auto_timer(0)  # Cancel auto timer
        stop()
        return False  # Stop listener

    if ignore_keypress:
        return

    new_velocity = {'x': 0, 'y': 0, 'zoom': 0}

    try:
        # Handle arrow keys
        if key == keyboard.Key.up:
            new_velocity = up.copy()
        elif key == keyboard.Key.down:
            new_velocity = down.copy()
        elif key == keyboard.Key.left:
            new_velocity = left.copy()
        elif key == keyboard.Key.right:
            new_velocity = right.copy()
        # Handle character keys
        elif hasattr(key, 'char'):
            if key.char == '-':
                new_velocity['zoom'] = -1
            elif key.char == '+' or key.char == '=':
                new_velocity['zoom'] = 1
            elif key.char == 'a':
                auto_mode = not auto_mode
                print(f"Auto Mode {auto_mode}")
                if auto_mode:
                    auto_count = 0
                    auto_cb()
                else:
                    schedule_auto_timer(0)  # Cancel auto timer
                    stop()
                return
    except AttributeError:
        pass

    if not auto_mode:
        move(new_velocity)


def main():
    """Main function."""
    global cam, ptz_service, profile_token

    # Read password
    password = read_password()

    # Connect to camera
    print(f"Connecting to camera at {HOSTNAME}:{PORT}...")
    try:
        cam = ONVIFCamera(HOSTNAME, PORT, USERNAME, password)

        # Get media service
        media_service = cam.create_media_service()

        # Get profiles
        profiles = media_service.GetProfiles()
        if not profiles:
            print("No media profiles found")
            sys.exit(1)

        # Use first profile
        profile = profiles[0]
        profile_token = profile.token

        # Get stream URI
        request = media_service.create_type('GetStreamUri')
        request.ProfileToken = profile_token
        request.StreamSetup = {'Stream': 'RTP-Unicast', 'Transport': {'Protocol': 'RTSP'}}
        stream_uri = media_service.GetStreamUri(request)

        print('------------------------------')
        print(f'Host: {HOSTNAME} Port: {PORT}')
        print(f'Stream: = {stream_uri.Uri}')
        print('------------------------------')

        # Create PTZ service
        ptz_service = cam.create_ptz_service()

    except Exception as e:
        print(f"Error connecting to camera: {e}")
        sys.exit(1)

    # Start keyboard listener
    print('')
    print('Use Arrow Keys to move camera. + and - to zoom. q to quit')
    print('Use a to toggle auto mode (repeated panning left and right forever)')

    with keyboard.Listener(on_press=on_press) as listener:
        listener.join()

    print("\nExiting...")
    sys.exit(0)


if __name__ == '__main__':
    main()
