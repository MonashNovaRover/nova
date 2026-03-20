import time

start_all()

rover.wait_for_unit("default.target")
base.wait_for_unit("default.target")

with subtest("both nets up"):
    rover.succeed("ip link set ethA up")
    rover.succeed("ip link set ethB up")

    time.sleep(1)

    rover.succeed("ping -c 1 -w 1 10.0.0.100")
    base.succeed("ping -c 1 -w 1 10.0.0.10")

    rover.succeed("ping -c 1 -w 1 10.5.0.100")
    base.succeed("ping -c 1 -w 1 10.5.0.10")

    rover.succeed("ping -c 1 -w 1 10.9.0.100")
    base.succeed("ping -c 1 -w 1 10.9.0.10")

with subtest("5GHz down"):
    rover.succeed("ip link set ethA down")
    rover.succeed("ip link set ethB up")

    time.sleep(1)

    rover.succeed("ping -c 1 -w 1 10.0.0.100")
    base.succeed("ping -c 1 -w 1 10.0.0.10")

    rover.fail("ping -c 1 -w 1 10.5.0.100")
    base.fail("ping -c 1 -w 1 10.5.0.10")

    rover.succeed("ping -c 1 -w 1 10.9.0.100")
    base.succeed("ping -c 1 -w 1 10.9.0.10")

with subtest("900MHz down"):
    rover.succeed("ip link set ethA up")
    rover.succeed("ip link set ethB down")

    time.sleep(1)

    rover.succeed("ping -c 1 -w 1 10.0.0.100")
    base.succeed("ping -c 1 -w 1 10.0.0.10")

    rover.succeed("ping -c 1 -w 1 10.5.0.100")
    base.succeed("ping -c 1 -w 1 10.5.0.10")

    rover.fail("ping -c 1 -w 1 10.9.0.100")
    base.fail("ping -c 1 -w 1 10.9.0.10")

with subtest("both down"):
    rover.succeed("ip link set ethA down")
    rover.succeed("ip link set ethB down")

    rover.fail("ping -c 1 -w 1 10.0.0.100")
    base.fail("ping -c 1 -w 1 10.0.0.10")

    rover.fail("ping -c 1 -w 1 10.5.0.100")
    base.fail("ping -c 1 -w 1 10.5.0.10")

    rover.fail("ping -c 1 -w 1 10.9.0.100")
    base.fail("ping -c 1 -w 1 10.9.0.10")

with subtest("no duplicate packets"):
    rover.succeed("ip link set ethA up")
    rover.succeed("ip link set ethB up")

    time.sleep(1)

    rover.fail("ping -c 3 -w 1 10.0.0.100 | grep -i dup")
    base.fail("ping -c 3 -w 1 10.0.0.10 | grep -i dup")

with subtest("ROS DDS only on prp0"):
    # we check for packets going to the multicast address the dds uses to discover
    # other computers.

    rover.succeed("ros2 daemon start")
    base.succeed("ros2 daemon start")

    rover.succeed("timeout 5 sudo tcpdump -i prp0 dst 239.255.0.1 --print -c 1 -n")
    base.succeed("timeout 5 sudo tcpdump -i prp0 dst 239.255.0.1 --print -c 1 -n")

    rover.fail("timeout 5 sudo tcpdump -i vlan9 dst 239.255.0.1 --print -c 1 -n")
    base.fail("timeout 5 sudo tcpdump -i vlan9 dst 239.255.0.1 --print -c 1 -n")

    rover.fail("timeout 5 sudo tcpdump -i vlan5 dst 239.255.0.1 --print -c 1 -n")
    base.fail("timeout 5 sudo tcpdump -i vlan5 dst 239.255.0.1 --print -c 1 -n")

with subtest("Everyone can talk to everyone on ros? and talk to themself?"):
    rover.execute("ros2 topic pub /rover/test std_msgs/String \"data: 'hello from rover'\" > /dev/null & disown; exit")
    base.execute("ros2 topic pub /base/test std_msgs/String \"data: 'hello from base'\" > /dev/null & disown; exit")
    
    base.succeed('ros2 topic echo /base/test std_msgs/String --once --timeout 2 | grep "hello from base"')
    rover.succeed('ros2 topic echo /base/test std_msgs/String --once --timeout 2 | grep "hello from base"')

    base.succeed('ros2 topic echo /rover/test std_msgs/String --once --timeout 2 | grep "hello from rover"')
    rover.succeed('ros2 topic echo /rover/test std_msgs/String --once --timeout 2 | grep "hello from rover"')
