#ifndef ENCODER_HEADER
#define ENCODER_HEADER

#include <string>
#include <algorithm>
#include <thread>
#include <gst/gst.h>

static int num_cores = std::thread::hardware_concurrency();

template <typename properties> void set_vp8enc(GstElement* encode, const properties props) {
  const int num_cores = std::thread::hardware_concurrency();
  g_object_set(encode,
    "deadline", props->deadline, // 1 for lowest latency
    "cpu-used", (
      props->cpu_used == 0 ? -16:
      props->cpu_used == 1 ? -12:
      props->cpu_used == 2 ? -8:
      props->cpu_used == 3 ? -5:
      props->cpu_used == 4 ? -2:
      props->cpu_used == 5 ? 0:
      props->cpu_used == 6 ? 2:
      props->cpu_used == 7 ? 5:
      props->cpu_used == 8 ? 8:
      props->cpu_used == 9 ? 12:
      props->cpu_used == 10 ? 16:
      0 ), // Fastest -16, 16 Slowest
    "end-usage", 1, // constant bitrate
    "threads", std::clamp(props->threads, 1, num_cores), // 1 is best for cpu and compression ratio
    "target-bitrate", std::clamp(props->bitrate, 1, 4096)*1000,
    "keyframe-max-dist", (int) props->gop * (int) ((float) props->framerate/ (float) props->framerate_denominator/ (float) props->downrate + 1.0), // Largest GOP
    "buffer-optimal-size", props->gop*1000,        // Buffer size for GOP
    "lag-in-frames", (props->deadline != 1) ? (int) ((float)props->deadline/(float)props->framerate) : 0, // Do not lookahead unless if deadline set
    "error-resilient", 1,
    "tuning", 1, // Tune for ssim, better for low bitrate/ blur
    NULL);
}


template <typename properties> void set_vp9enc(GstElement* encode, const properties props) {
  g_object_set(encode,
    "deadline", props->deadline, // 1 for lowest latency
    "cpu-used", (
      props->cpu_used == 0 ? -16:
      props->cpu_used == 1 ? -12:
      props->cpu_used == 2 ? -8:
      props->cpu_used == 3 ? -5:
      props->cpu_used == 4 ? -2:
      props->cpu_used == 5 ? 0:
      props->cpu_used == 6 ? 2:
      props->cpu_used == 7 ? 5:
      props->cpu_used == 8 ? 8:
      props->cpu_used == 9 ? 12:
      props->cpu_used == 10 ? 16:
      0 ), // Fastest -16, 16 Slowest
    "end-usage", 1, // constant bitrate
    "threads", std::clamp(props->threads, 1, num_cores), // 1 is best for cpu and compression ratio
    "target-bitrate", std::clamp(props->bitrate, 1, 4096)*1000,
    "keyframe-max-dist", (int) props->gop * (int) ((float) props->framerate/ (float) props->framerate_denominator/ (float) props->downrate + 1.0), // Largest GOP
    "buffer-optimal-size", props->gop*1000,        // Buffer size for GOP
    "lag-in-frames", (props->deadline != 1) ? (int) ((float)props->deadline/(float)props->framerate) : 0, // Do not lookahead unless if deadline set
    "error-resilient", 1,
    "tuning", 1, // Tune for ssim, better for low bitrate/ blur
    "aq-mode", 3, // cyclic refresh aq mode, low latency low bitrate
    NULL);
}

#endif
