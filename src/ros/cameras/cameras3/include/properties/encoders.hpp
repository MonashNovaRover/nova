#ifndef ENCODER_HEADER
#define ENCODER_HEADER

#include <string>
#include <algorithm>
#include <thread>
#include <gst/gst.h>

static int num_cores = std::thread::hardware_concurrency();

template <typename properties> static void set_h26Xenc(GstElement* element, const properties& props) {
  g_object_set(element,
    "bitrate", std::clamp(props->bitrate, 1, 4096),
    "key-int-max", (int) props->gop * (int) ((float) props->framerate/ (float) props->framerate_denominator/ (float) props->downrate + 1.0), // Largest GOP
    "qos", true,
    "speed-preset", std::clamp(props->cpu_used, 1, 10), // Fastest 1, 10 Slowest
    "tune", 4, // Zero latency decode
    NULL);
}

template <typename properties> static void set_h264enc(GstElement* element, const properties& props) {
  set_h26Xenc(element, props);
  g_object_set(element,
    "analyse", 2, // i8x8
    "b-adapt", false, // Do not add b frames
    "bframes", 0, // Never have b frames
    "dct8x8", true,
    "ip-factor", 1.7, // Less priority on I frames
    "noise-reduction", std::clamp(props->encoder_denoise * 1000, 0, 100000), // higher is more blurry
    "option-string", "aq-mode=2:aq-strength=1.0:deblock=-3,-3",
    "pass", 0, // cbr
    "psy-tune", 5, // ssim, better for humans
    "rc-lookahead", std::clamp(props->deadline, 0, 250), // how far to delay frames for quality
    "sliced-threads", false, // Never enable, keep cpu usage lower
    "threads", std::clamp(1<<props->threads, 1, num_cores), // Always single thread for scalability
    "vbv-buf-capacity", std::clamp(props->bitrate*3, 2000, 10000),
  NULL);
}

template <typename properties> static void set_h265enc(GstElement* element, const properties& props) {
  set_h26Xenc(element, props);
  g_object_set(element,
    "option-string", "frame-threads=1:pools=none:repeat-headers=1",
  NULL);
}


template <typename properties> static void set_vpXenc(GstElement* element, const properties& props) {
  g_object_set(element,
    "buffer-optimal-size", props->gop*1000,        // Buffer size for GOP
    "cpu-used", (
      props->cpu_used == 10 ? 0:
      props->cpu_used == 9 ? 2:
      props->cpu_used == 8 ? 4:
      props->cpu_used == 7 ? 6:
      props->cpu_used == 6 ? 7:
      props->cpu_used == 5 ? 8:
      props->cpu_used == 4 ? 9:
      props->cpu_used == 3 ? 10:
      props->cpu_used == 2 ? 12:
      props->cpu_used == 1 ? 14:
      props->cpu_used == 0 ? 16:
    0 ), // Fastest 0, 16 Slowest
    "deadline", props->deadline, // 1 for lowest latency
    "dropframe-threshold", 99, // Drop frames as a last resort if bitrate not met
    "end-usage", 1, // constant bitrate
    "error-resilient", 1, // Whole frame
    "horizontal-scaling-mode", 3, // 50% scaling when resizing
    "keyframe-max-dist", (int) props->gop * (int) ((float) props->framerate/ (float) props->framerate_denominator/ (float) props->downrate + 1.0), // Largest GOP
    "lag-in-frames", (props->deadline != 1) ? (int) ((float)props->deadline/(float)props->framerate) : 0, // Do not lookahead unless if deadline set
    "noise-sensitivity", std::clamp(props->encoder_denoise, 0, 6), // higher is more blurry
    "overshoot", 20, // Do not tolerate overshooting much over the target bitrate
    "qos", true,
    "resize-allowed", true,
    "resize-down-threshold", 10,
    "resize-up-threshold", 90,
    "static-threshold", std::clamp(100, 1, 100), // Higher to stop updating screen if too little moves
    "target-bitrate", std::clamp(props->bitrate, 1, 4096)*1000,
    "temporal-scalability-number-layers", 4, // Will downgrade fps 3 times to keep bitrate
    "threads", std::clamp(1<<props->threads, 1, num_cores), // 1 thread is most efficient. Log2
    "tuning", 1, // Tune for ssim, better for low bitrate/ blur
    "vertical-scaling-mode", 3, // 50% scaling when resizing
  NULL);
}

template <typename properties> void set_vp8enc(GstElement* element, const properties& props) {
  set_vpXenc(element, props);
}


template <typename properties> void set_vp9enc(GstElement* element, const properties& props) {
  set_vpXenc(element, props);
  g_object_set(element,
    "aq-mode", 3, // cyclic refresh aq mode, low latency low bitrate
    "tile-columns", std::clamp(props->threads, 0, 6), // 0-6 for 1-64 columns
  NULL);
}

#endif
