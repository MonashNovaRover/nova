#include <string>
#include <algorithm>
#include <thread>
#include <gst/gst.h>
#include "properties/software_encoders.hpp"

static int num_cores = std::thread::hardware_concurrency();

void set_av1enc(GstElement* encode, const int cpu_used, const int threads, const int bitrate, const int gop, const int framerate, const int framerate_denominator, const int downrate) {
    g_object_set(encode,
        "cpu-used", std::clamp(11-cpu_used, 0, 10), // Fastest 10, 1 Slowest 
        "end-usage", 1, // constant bitrate
        "usage-profile", 1, // realtime 
        "threads", std::clamp(threads, 1, num_cores), // 1 is best for cpu and compression ratio
        "target-bitrate", std::clamp(bitrate, 1, 4096),
        "keyframe-max-dist", (int) gop * (int) ((float) framerate/ (float) framerate_denominator/ (float) downrate + 1.0), // Largest GOP
        "tile-columns", std::clamp(threads, 1, 6),
        "tile-rows", std::clamp(threads, 1, 6),
        NULL);
}

void set_vp8enc(GstElement* encode, const int cpu_used, const int threads, const int bitrate, const int gop, const int framerate, const int framerate_denominator, const int downrate) {
    g_object_set(encode,
        "deadline", 1, // 1 for lowest latency
        "cpu-used", (
          cpu_used == 0 ? -16:
          cpu_used == 1 ? -12:
          cpu_used == 2 ? -8:
          cpu_used == 3 ? -5:
          cpu_used == 4 ? -2:
          cpu_used == 5 ? 0:
          cpu_used == 6 ? 2:
          cpu_used == 7 ? 5:
          cpu_used == 8 ? 8:
          cpu_used == 9 ? 12:
          cpu_used == 10 ? 16:
          0 ), // Fastest -16, 16 Slowest 
        "end-usage", 1, // constant bitrate
        "threads", std::clamp(threads, 1, num_cores), // 1 is best for cpu and compression ratio
        "target-bitrate", std::clamp(bitrate, 1, 4096)*1000,
        "keyframe-max-dist", (int) gop * (int) ((float) framerate/ (float) framerate_denominator/ (float) downrate + 1.0), // Largest GOP
        "buffer-optimal-size", gop*1000,        // Buffer size for GOP
        "lag-in-frames", 0, // Do not lookahead
        "error-resilient", 1,
        NULL);
}


void set_vp9enc(GstElement* encode, const int cpu_used, const int threads, const int bitrate, const int gop, const int framerate, const int framerate_denominator, const int downrate) {
    g_object_set(encode,
        "deadline", 1, // 1 for lowest latency
        "cpu-used", (
          cpu_used == 0 ? -16:
          cpu_used == 1 ? -12:
          cpu_used == 2 ? -8:
          cpu_used == 3 ? -5:
          cpu_used == 4 ? -2:
          cpu_used == 5 ? 0:
          cpu_used == 6 ? 2:
          cpu_used == 7 ? 5:
          cpu_used == 8 ? 8:
          cpu_used == 9 ? 12:
          cpu_used == 10 ? 16:
          0 ), // Fastest -16, 16 Slowest 
        "end-usage", 1, // constant bitrate
        "threads", std::clamp(threads, 1, num_cores), // 1 is best for cpu and compression ratio
        "target-bitrate", std::clamp(bitrate, 1, 4096)*1000,
        "keyframe-max-dist", (int) gop * (int) ((float) framerate/ (float) framerate_denominator/ (float) downrate + 1.0), // Largest GOP
        "buffer-optimal-size", gop*1000,        // Buffer size for GOP
        "lag-in-frames", 0, // Do not lookahead
        "error-resilient", 1,
        "aq-mode", 5, // perceptual aq mode
        "tile-columns", 0,
        "tile-rows", 0,
        NULL);
}

void set_x264enc(GstElement* encode, const int cpu_used, const int threads, const int bitrate, const int gop, const int framerate, const int framerate_denominator, const int downrate) {
    g_object_set(encode,
        "tune", 0x00000004, // zerolatency
        "speed-preset", std::clamp(cpu_used, 1, 10), 
        "threads", std::clamp(threads, 1, num_cores), // 1 is best for cpu and compression ratio
        "bitrate", std::clamp(bitrate, 1, 4096),
        "key-int-max", (int) gop * (int) ((float) framerate/ (float) framerate_denominator/ (float) downrate + 1.0), // Largest GOP
        "vbv-buf-capacity", gop*1125,        // Buffer size for GOP
        "b-adapt", false, // Do not allow b frames
        "sliced-threads", false, // Do not sacrifice cpu usage for lower latency
        NULL);
}

