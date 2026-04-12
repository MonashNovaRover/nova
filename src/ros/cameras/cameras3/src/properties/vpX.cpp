#include <string>
#include <gst/gst.h>
#include "properties/vpX.hpp"

void set_vpXenc(GstElement* encode, const int deadline, const int cpu_used, const std::string end_usage, const int threads, const int bitrate, const int gop, const int framerate, const int framerate_denominator, const int downrate, const std::string video_caps, const std::string aq_mode) {
    g_object_set(encode,
        "deadline", deadline, // 1 for lowest latency
        "cpu-used", cpu_used, // Fastest -16, 16 Slowest 
        "end-usage", (
            end_usage == "vbr" ? 0:
            end_usage == "cbr" ? 1:
            end_usage == "cq" ? 2:
            1), // mode, constant bitrate best
        "threads", threads, // 1 is best for cpu and compression ratio
        "target-bitrate", bitrate*1000,
        "keyframe-max-dist", (int) gop * (int) ((float) framerate/ (float) framerate_denominator/ (float) downrate + 1.0), // Largest GOP
        "buffer-optimal-size", gop*1000,        // Buffer size for GOP
        "lag-in-frames", 0, // Do not lookahead
        "error-resilient", 1,
        NULL);
    
    if (video_caps == "video/x-vp9") {
        g_object_set(encode,
            "aq-mode", (
                aq_mode == "off" ? 0 :
                aq_mode == "variance" ? 1 :
                aq_mode == "complexity" ? 2 :
                aq_mode == "cyclic-refresh" ? 3 :
                aq_mode == "equator360" ? 4 :
                aq_mode == "perceptual" ? 5 :
                aq_mode == "psnr" ? 6 :
                aq_mode == "lookahead" ? 7 :
                5),
            "tile-columns", threads,
            "tile-rows", threads,
          NULL);
    }
}
