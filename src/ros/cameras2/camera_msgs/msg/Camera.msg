# name of camera
string serial
# location of camera (e.g. /dev/videoX, /tmp/oak_L_out)
string node
# hint for gstreamer as to what caps to get from this camera (optional)
string outputcaps # for gst_caps_from_string()
