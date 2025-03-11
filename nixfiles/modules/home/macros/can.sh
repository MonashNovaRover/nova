# +--------------------------------------------+
#               MONASH NOVA ROVER
# +--------------------------------------------+
#
# This script runs CAN, either virtually or on
# a real bus. This must be executed before any
# of the CAN modules can be used.
#
# Some examples could be:
#   can start can0
#   can stop vcan0
#
# The options are:
#   drive -  CAN 0 Line 250k
#   ec    -  CAN 0 Line 250k 
#   arm   -  CAN 1 Line 200k
#   sci   -  CAN 1 Line 250k
#
#   can0 -  CAN 0 Line
#   can1 -  CAN 1 Line
#   can2 -  CAN 2 Line
#   all -   CAN 0, CAN 1 and CAN2 Line
#
#   vcan0 - Virtual CAN 0 Line
#   vcan1 - Virtual CAN 1 Line
#
# +--------------------------------------------+

# Add the colors
ERROR='\033[0;31;1m'
END='\033[0m'

# Create an error function
information() {
    printf "%b%s%b\n" "$ERROR" "$1" "$END"
}

# Reset the failed flag
failed=0

# Validate command argument
if [[ "$1" == "start" || "$1" == "stop" ]]; then
    command="$1"
else
    information "Invalid Command! Please enter 'start' or 'stop'."
    exit 1
fi

# Determine the CAN interface
case "$2" in
    can0|drive|ec) can="can0" ; bitrate=250000 ;;
    can1|arm|sci) can="can1" ; bitrate=250000 ;;
    can2) can="can2" ; bitrate=250000 ;;
    vcan0) can="vcan0" ; bitrate=250000 ;;
    vcan1) can="vcan1" ; bitrate=250000 ;;
    all)
        "$0" "$command" "can0"
        "$0" "$command" "can1"
        "$0" "$command" "can2"
        exit 0 ;;
    *)
        information "Incorrect CAN line. Please enter one of:\n	['drive', 'ec', 'arm', 'sci', 'can0', 'can1', 'can2', 'all', 'vcan0', 'vcan1']"
        exit 1 ;;
esac

# If bitrate parameter is provided, override default
if [[ -n "$3" ]]; then
    bitrate="$3"
fi

# Print message
echo "Attempting to $command $can..."

# Virtual CAN Handling
if [[ "${can:0:1}" == "v" ]]; then
    sudo modprobe vcan
    if [[ "$command" == "start" ]]; then
        sudo ip link add dev "$can" type vcan
        sudo ip link set up "$can"
    else
        sudo ip link set down "$can"
    fi
    exit 0
fi

# Real CAN Handling
sudo modprobe can
sudo modprobe can-raw
sudo modprobe mttcan

if [[ "$command" == "start" ]]; then
    sudo ip link set "$can" type can bitrate "$bitrate" berr-reporting on
    sudo ip link set up "$can"
else
    sudo ip link set down "$can"
fi
