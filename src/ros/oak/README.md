

# run the main script
nix-shell shell.nix
python ./oakenc.py

# signalling server (if not already made by cameras2
nix-shell gstreamer.nix
gst-webrtc-signalling-server


# pipe a usb camera into the oak
nix-shell gstreamer.nix
gst-launch-1.0 v4l2src device=/dev/video2 ! "image/jpeg, width=640,framerate=10/1" ! udpsink port=4996 host=127.0.0.1


# get encoded streams out of the oak
nix-shell gstreamer.nix
for port in 5000 5001 5002 5003; do
    gst-launch-1.0 -v udpsrc port=$port ! video/x-h264  ! queue ! webrtcsink do-fec=true do-retransmission=true congestion-control=gcc meta='meta, serial=(string)'$port &
done


# issues

Didn't get encoded depth output working. - Need to make a hostnode to convert greyscale to NV12

When any of your gst udpsinks stop, the oak stops as it wants all the inputs to be in sync. may be able to solve this by repeating the last good frame till we get new frames.


udpsink for gst can't do large frames because of udp packet size.   rndbuffersize max=20000 min=19000  stops the errors, but adds to latency i think. OAK can't convert large mjpeg to NV12 so maybe we just have to do that on the orin side. Can't do that in gst because udp packet size limits... if we can package gst-namedpipe I think that would make this a lot nicer.


need to test how many usb cams the oak can encode at once at what resolutions

You cannot change the resolution of any of the encoder nodes without restarting the whole thing.




