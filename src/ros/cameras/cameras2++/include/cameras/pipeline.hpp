#include <string>

struct Properties
{
  std::string serial;
  std::string node;
};

struct v4lProperties
{
  int width;
  int height;
  int framerate;
  std::string mime;
};

struct webRTCProperties
{
  std::string video_caps;
  bool do_fec;
  bool do_retransmission;
  std::string congestion_control;
};

struct x264encProperties
{
  std::string tune;
  std::string speed_preset;
  int bitrate;
  bool byte_stream;
  std::string me;
  int threads;

  std::string alignment;
  std::string stream_format;
  std::string format;
};

struct x265encProperties
{
  std::string tune;
  std::string speed_preset;
};

struct clockProperties
{
  bool show_clock;
};
