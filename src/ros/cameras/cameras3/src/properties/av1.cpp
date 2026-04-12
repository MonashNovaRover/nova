#include <string>
#include <gst/gst.h>
#include "properties/av1.hpp"

void set_av1enc(GstElement* encode, const int cpu_used, const std::string end_usage, const std::string usage_profile, const int threads, const int bitrate, const int gop, const int framerate, const int framerate_denominator, const int downrate) {
    g_object_set(encode,
        "cpu-used", cpu_used, // Fastest 10, 1 Slowest 
        "end-usage", (
            end_usage == "vbr" ? 0:
            end_usage == "cbr" ? 1:
            end_usage == "cq" ? 2:
            1), // mode, constant bitrate best
        "usage-profile", (
            usage_profile == "good-quality" ? 0:
            usage_profile == "realtime" ? 1:
            usage_profile == "all-intra" ? 2:
            2), 
        "threads", threads, // 1 is best for cpu and compression ratio
        "target-bitrate", bitrate,
        "keyframe-max-dist", (int) gop * (int) ((float) framerate/ (float) framerate_denominator/ (float) downrate + 1.0), // Largest GOP
        "tile-columns", threads,
        "tile-rows", threads,
        NULL);
}
