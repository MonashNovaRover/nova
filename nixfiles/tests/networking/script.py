
rover.wait_for_unit("default.target")
base.wait_for_unit("default.target")

with subtest("Ping both ways"):
    #init_graphical()
    rover.succeed("ping -c 1 -w 1 10.0.0.1")
    base.succeed("ping -c 1 -w 1 10.0.0.10")

