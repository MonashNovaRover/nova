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

struct clockProperties
{
    bool show_clock;
};