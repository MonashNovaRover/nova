import yaml

start_all()

rover.wait_for_unit("default.target")
rover.wait_for_unit("nova-can-sleuth.service")

with subtest("state file created"):
    rover.succeed("cat /run/can_sleuth_state")

