#!/bin/bash

# Set the DSCP priority for ports in the range 7400-32649 to the "video" priority (CS5). Packets will try to be delivered within 100ms.
# https://phoenixnap.com/kb/iptables-tutorial-linux-firewall 
sudo iptables -t mangle -A OUTPUT -p udp -j DSCP --dport 7400:10000 --set-dscp-class CS05
