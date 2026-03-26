# cameras.hpp
In cameras.hpp, the directory for all services is defined. These are used throughout camera_directory_service.cpp and camera_streamer_service.cpp.

# pipelines.hpp
In pipelines.hpp, the tunable properties of custom pipelines are defined. To replicate cameras2, use the defaults of v4l2webrtc.


## Properties
This is the general id given to every camera. They are split into:

### serial
The usb address of the camera by name. This is the name the camera broadcasts to v4l2, and can be remapped.

Examples:
- webcamvendor_SK_series1_20220325JWGD2093
- brandon-cam
- mast_forward

### node
The device directory of the camera. This can be found with:
v4l2-ctl -A

Example:
- /dev/video0
- /dev/video4


## v4lProperties
The properties given to the gstreamer element `v4l2src`.

### width

### height

### framerate

### brightness

### contrast

### device

### mime

### v4l2_controls

### crop43


## webRTCProperties
The properties given to the gstreamer element `webrtcsink`.

### video_caps



struct webRTCProperties
{
  std::string video_caps;
  bool do_fec;
  bool do_retransmission;
  std::string congestion_control;
};

struct h264passthroughProperties
{
  bool payload_quirk;
};

struct x264encProperties
{
  std::string tune;
  std::string speed_preset;
  std::string me;
  int subme;
  int threads;
  int bitrate;
  int noise_reduction;
  int gop;
  std::string decoder;
};

struct clockProperties
{
  bool show_clock;
};

struct Pipeline
{
  GstElement* gst_pipeline;
  Properties* props;
  std::string pipeline_type;
  camera_msgs::msg::Camera* camera;
  std::string profile;
};

struct v4l2webrtcPipelineProperties : Properties, v4lProperties, webRTCProperties, clockProperties {};
v4l2webrtcPipelineProperties* get_v4l2webrtc_pipeline_properties(rclcpp::Node* log_node, camera_msgs::msg::Camera* camera);
GstElement* v4l2webrtc_pipeline(rclcpp::Node* log_node, v4l2webrtcPipelineProperties* props);
