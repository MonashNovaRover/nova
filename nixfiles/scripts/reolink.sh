# open reolink video stream

set -euo pipefail

#TODO: remove once nix is working properly
if which ffplay &>/dev/null; then
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
URI=""
FILENAME=""
OP="stream"

# some of these might not really do anything.
# there are some more mentioned in https://trac.ffmpeg.org/wiki/StreamingGuide that I haven't had time to test.
LOW_LATENCY_ARGS="-flags low_delay -fflags nobuffer -framedrop -rc_lookahead 0 -probesize 32 -analyzeduration 0 -vf setpts=0 -tune zerolatency"

FILENAME=""
while test $# -gt 0; do
  case "$1" in
    high) STREAM_NAME="Preview_01_main"
      ;;
    low) STREAM_NAME="Preview_01_sub"
      ;;
    180) FLIP="-vf hflip,vflip"
      ;;
    "-o") FILENAME=$2
      shift
      ;;
    stream) OP="stream"
      ;;
    save) OP="save"
      ;;
    "--uri") URI=$2
      shift
      ;;
    *) echo Usage: $0 "[high|low] [save|stream] [180] [-o OUTPUT_FILENAME]"
      exit
      ;;
  esac
  shift
done

if [ -e "$FILENAME" ]; then
  echo "$FILENAME" already exists!
  exit 1
fi

if [ -z "$FILENAME" ]; then
  # set default timebased filename
  FILENAME="$HOME/recordings/reolink_$(date -Iseconds | tr : _).mp4"
fi

if [ -z "$URI" ]; then
  URI="rtsp://$USERNAME:$PASSWORD@$IP:$PORT/$STREAM_NAME"
fi

if [ "stream_" == "$OP"_ ]; then
  # we are streaming to a window
  exec ffplay -rtsp_transport udp -i "$URI" \
    $FLIP $LOW_LATENCY_ARGS -window_title "$NAME"
elif [ "save_" == "$OP"_ ]; then
  # we are saving to a file
  mkdir -p "$(dirname $FILENAME)"
  echo "Saving to $FILENAME"

  # carefully handle errors here so we still print filename if user
  # stops ffmpeg with ctrl-c instead of bash exiting immediately
  set +e
  ffmpeg -i "$URI" -vcodec copy -acodec copy $FLIP "$FILENAME"
  RET=$?
  set -e
  echo "Saved to $FILENAME"
  echo "To copy to your laptop: scp $USER@$(hostname -I | tr -d ' '):$FILENAME ./"
  exit $RET
else
  echo invalid op $OP
  exit 1
fi