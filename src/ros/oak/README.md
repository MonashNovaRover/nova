

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

Didn't get encoded depth output working.

When any of your gst udpsinks stop, the oak stops as it wants all the inputs to be in sync. may be able to solve this by repeating the last good frame till we get new frames.
Running usb cameras to the oak doesn't work well


udpsink for gst can't do large frames because of udp packet size.   rndbuffersize max=20000 min=19000  stops the errors, but adds to latency i think. OAK can't convert large mjpeg to NV12 so maybe we just have to do that on the orin side. Can't do that in gst because udp packet size limits... if we can package gst-namedpipe I think that would make this a lot nicer.


# conclusions

we can run the 3 rgb cams on an oak well, at full res 30fps they are about 2 megabytes per second each. You could even add mirrors to make them point in different directions.


making the oak do our encoding is weird. one camera is usually ok up to 15fps as long as the resolution isn't too high (1280x720 too big, 1024x576 is ok, the reason for resoultion being restricted is the RGB->NV12 step on the oak, tried to do this on computer but didn't have cv installed so failed to write a working converter. if you fix the udp packet size limit then you could do that step in gst.).

having a usb camera along side the oak cameras makes the usb camera have very high latency (several seconds).


I think it would be good to have 1 oak on the arm, ec, science etc as a camera, and then another in the chassis as an encoder. Need to investigate more around how much encoding we can do of usb cameras. Might get better performance if we get 4 camera feeds, put them in a grid, then encode them together, but not sure.

I really want to make it do red-cyan 3d glasses mode. this would require getting R from left cam, BG from right cam, and putting them together into RGB. https://docs.luxonis.com/software-v3/depthai/tutorials/on-device-programming/#On%20Device%20Programming-Creating%20custom%20NN%20models maybe with this you could do it...



