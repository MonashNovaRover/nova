#ifndef PROPERTIES_HEADER
#define PROPERTIES_HEADER

#include <string>
#include <memory>
#include <gst/gst.h>
#define PIPELINE_PREFIX         "serial_pipelines"
#define PROFILE_PREFIX          "profiles"
#define PRESET_PREFIX           "presets"
#define UNKNOWN_PROFILE_PREFIX  "unknown"
#define DEFAULT_PREFIX          "defaults"

struct Properties
{
  std::string serial;
  std::string node;
  std::string original_serial;

  bool use_gl;
};

struct v4lProperties
{
  std::string device;
  std::string io_mode;

  bool strict_devname;
};

struct capsProperties
{
  std::string format;
  std::string mime;

  int brightness;
  int contrast;
  int saturation;
  int gain;
  int sharpness;

  int downrate;
  int downscale;
  int height;
  int framerate;
  int framerate_denominator;
  int width;
};

struct h264PassthroughProperties
{
  bool payload_quirk;
};

struct glProperties
{
  float denoise_factor;
  float denoise_sigma;
  float denoise_threshold;
  int denoise_radius;
  float sharpen_radius;
  float sharpen_strength;
  float undistort_k1;
  float undistort_k2;
  float undistort_scale;

  bool denoise;
  bool sharpen;
  bool undistort;
};

struct softwareEncProperties
{
  std::string end_usage;
  std::string usage_profile;

  int cpu_used;
  int deadline;
  int gop;
  int encoder_denoise;
  int threads;
};

struct webRTCProperties
{
  std::string congestion_control;
  std::string video_caps;

  int bitrate;

  bool do_fec;
  bool do_retransmission;
};

struct decodeProperties
{
  std::string decoder;
  std::string jpegdec_method;
};

struct cpuFiltersProperties
{
  std::string chroma_resampler;
  std::string dither;
  std::string method;

  float zoom;
  float zoom_longitude;
  float zoom_latitude;

  bool crop43;
};

struct clockProperties
{
  bool show_clock;
};

struct rossinkProperties
{
  std::string ros_format;
  std::string ros_topic;

  bool rossink;
};


struct Pipeline
{
  GstElement* gst_pipeline;
  std::unique_ptr<camera_msgs::msg::Camera> camera;
};

#endif
