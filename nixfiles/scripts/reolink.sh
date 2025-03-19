# open reolink video stream

set -euo pipefail

#TODO: remove once nix is working properly
if which ffplay 2>/dev/null; then
  :
else
  echo "Please run me in a \`nix-shell -p ffmpeg\`"
  exit 1
fi


IP="10.0.1.100"
PORT="554"
USERNAME="admin"
PASSWORD="Lab188b37" # TODO don't have this here
NAME="Banksia Cam"
STREAM_NAME="Preview_01_main"

FLIP=""

while test $# -gt 0; do
  case "$1" in
    high) STREAM_NAME="Preview_01_main"
      ;;
    low) STREAM_NAME="Preview_01_sub"
      ;;
    180) FLIP="-vf hflip,vflip"
      ;;
    *) echo Usage: $0 "[high|low] [180]"
      exit
      ;;
  esac
  shift
done

# some of these might not really do anything.
LOW_LATENCY_ARGS="-flags low_delay -fflags nobuffer -framedrop -rc_lookahead 0 -probesize 32 -analyzeduration 0 -vf setpts=0 -tune zerolatency"

exec ffplay -rtsp_transport udp \
  -i rtsp://$USERNAME:$PASSWORD@$IP:$PORT/$STREAM_NAME \
  $FLIP $LOW_LATENCY_ARGS -window_title "$NAME"

