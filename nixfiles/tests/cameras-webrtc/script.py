import yaml

rover.wait_for_unit("default.target")
rover.wait_for_unit("nova-mock-cameras.service")

with subtest("Launch the camera services"):
    rover.succeed("ros2 launch --noninteractive cameras2 camera_server_launch.py param-dir:=\"$(mktemp -d)\" >&2 &")

with subtest("Check the camera list"):
    def check_camera_list(last: bool) -> bool:
        # The camera capabilities do not update appropriately. Trigger the
        # relevant rules manually.
        rover.succeed("udevadm trigger --subsystem-match=video4linux --attr-match=max_openers='?*'")

        cameras_yaml = rover.succeed("ros2 topic echo --once --full-length --qos-reliability reliable --qos-durability transient_local camera_directory/cameras camera_msgs/Cameras")
        rover.log(cameras_yaml)
        cameras = next(yaml.load_all(cameras_yaml, Loader=yaml.CLoader))["cameras"]

        # Check that all the expected cameras are present, and make sure that
        # their serial numbers are correct.
        if set(camera["serial"] for camera in cameras) != set(f"camera{i}" for i in range(1, camera_count + 1)):
            return False
        for camera in cameras:
            assert camera["node"] == f"/dev/video{camera['serial'][len('camera'):]}"

        return True

    # The cameras will not be visible until the producer process has attached to
    # them. Keep trying until they all show up.
    retry(check_camera_list)

with subtest("Start streaming the cameras"):
    rover.succeed("ros2 service call camera_streamer/stream/start camera_msgs/CameraOperation '{ serials: [ ] }'")
    rover.succeed("gst-webrtc-ui-server >&2 &")

with subtest("Check the camera stream playback"):
    # Start the browser
    init_graphical()
    base.succeed(f"{run_graphical('firefox --kiosk rover:8000')} >&2 &")

    def check_text(last: bool) -> bool:
        text = base.get_screen_text()
        return all(f"Test stream {i}" in text for i in range(1, camera_count + 1))

    with base.nested("Waiting for all streams to connect"):
        retry(check_text)

base.screenshot("finished")