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
};

struct x265encProperties
{
  std::string tune;
  std::string speed_preset;
  int bitrate;
};

struct clockProperties
{
    bool show_clock;
};
