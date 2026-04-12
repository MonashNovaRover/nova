#include <string>
#include <gst/gst.h>
#include "properties/h264.hpp"

void set_x264enc(GstElement* encode, const std::string tune, const std::string speed_preset, const int threads, const int bitrate, const int noise_reduction, const int gop, const int framerate, const int framerate_denominator, const int downrate) {
    g_object_set(encode,
        "tune", ( 
            tune == "stillimage" ? 0x00000001:
            tune == "fastdecode" ? 0x00000002:
            tune == "zerolatency" ? 0x00000004:
            0x00000004), // zerolatency
        "speed-preset", (
            speed_preset == "None" ? 0:
            speed_preset == "ultrafast" ? 1:
            speed_preset == "superfast" ? 2:
            speed_preset == "veryfast" ? 3:
            speed_preset == "faster" ? 4:
            speed_preset == "fast" ? 5:
            speed_preset == "medium" ? 6:
            speed_preset == "slow" ? 7:
            speed_preset == "slower" ? 8:
            speed_preset == "veryslow" ? 9:
            speed_preset == "placebo" ? 10:
            1), // ultrafast 
        "threads", threads, // 1 is best for cpu and compression ratio
        "bitrate", bitrate,
        "noise-reduction", noise_reduction,
        "key-int-max", (int) gop * (int) ((float) framerate/ (float) framerate_denominator/ (float) downrate + 1.0), // Largest GOP
        "vbv-buf-capacity", gop*1000,        // Buffer size for GOP
        "b-adapt", false, // Do not allow b frames
        "sliced-threads", false, // Do not sacrifice cpu usage for lower latency
        NULL);
}

void set_h264payload(GstElement* payload, const bool payload_quirk) {
    if (payload_quirk) {
        // Apply patch for gc2093
        g_object_set(payload,
            "aggregate-mode", 1,
            "config-interval", -1,
            NULL);
    }
}

void set_h264parse(GstElement* parse, const int interval) {
    g_object_set(parse,
        "config-interval", interval,
        NULL);
}

