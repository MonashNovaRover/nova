
import time

start_all()

rover.wait_for_unit("default.target")
rover.wait_for_unit("nova-can-sleuth.service")


# wait for it to start logging
for i in range(10):
    ret, _ = rover.execute("cat /run/can_sleuth_state")
    if ret == 0:
        break
    time.sleep(1)

with subtest("twitch FLD"):
    rover.succeed("jq .FLD.BLCMDEmulator.Pos.value /run/can_sleuth_state | grep +000.0")
    rover.succeed("cansend can0 011#")
    time.sleep(1)
    rover.fail("jq .FLD.BLCMDEmulator.Pos.value /run/can_sleuth_state | grep +000.0")

# setup ssh for launch script to use
with subtest("Setup Base->Rover SSH"):
    base.succeed("sudo -u nova ssh-keygen -t rsa -N '' -f /home/nova/.ssh/id_rsa")
    base_pubkey = base.execute("cat /home/nova/.ssh/id_rsa.pub")[1].strip()
    rover.succeed("sudo -u nova mkdir -m 0700 /home/nova/.ssh")
    rover.succeed(f"sudo -u nova echo {base_pubkey} >> /home/nova/.ssh/authorized_keys")
    base.succeed("sudo -u nova ssh-keyscan rover >> /home/nova/.ssh/known_hosts")
    base.succeed("sudo -u nova ssh rover echo HI")

    for vm in (base, rover):
        vm.succeed("sudo -u nova mkdir /home/nova/Builds")
        vm.succeed("sudo -u nova ln -s $(dirname $(readlink $(which ros2)))/.. /home/nova/Builds/master")

        vm.succeed("sudo -iu nova use_fastdds") #FIXME why no cyclone

asNova = "sudo -iu nova "
with subtest("Everyone can talk to everyone on ros? and talk to themself?"):
    rover.succeed(asNova+"ros2 daemon start")
    base.succeed(asNova+"ros2 daemon start")

    rover.execute(asNova+"ros2 topic pub /rover/test std_msgs/String \"data: 'hello from rover'\" > /dev/null & disown; exit")
    base.execute(asNova+"ros2 topic pub /base/test std_msgs/String \"data: 'hello from base'\" > /dev/null & disown; exit")
    
    base.succeed(asNova+'ros2 topic echo /base/test std_msgs/String --once --timeout 15 | grep "hello from base"')
    rover.succeed(asNova+'ros2 topic echo /base/test std_msgs/String --once --timeout 15 | grep "hello from base"')

    base.succeed(asNova+'ros2 topic echo /rover/test std_msgs/String --once --timeout 15 | grep "hello from rover"')
    rover.succeed(asNova+'ros2 topic echo /rover/test std_msgs/String --once --timeout 15 | grep "hello from rover"')


with subtest("launch drive"):
    base.succeed(f"{run_graphical('/home/nova/Builds/active/launch/run-drive rover')} >&2 &")

    # press unlock and forwards!
    base.succeed("sudo -iu nova ros2 topic pub -t 10 /drive/joy sensor_msgs/msg/Joy '{axes: [0.0, 1.0, 0.0, 0.0, 0.0, 0.0], buttons: [0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0]}'")

    # should have moved
    rover.fail("jq .BLD.BLCMDEmulator.Pos.value /run/can_sleuth_state | grep +000.0")

