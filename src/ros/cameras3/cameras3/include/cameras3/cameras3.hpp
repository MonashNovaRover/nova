#include "rclcpp/rclcpp.hpp"

#define SERVICE_START       "/camera_streamer/stream/start"
#define SERVICE_STOP        "/camera_streamer/stream/stop"
#define SERVICE_PAUSE       "/camera_streamer/stream/pause"
#define SERVICE_STATS       "/camera_streamer/stream/get_stats"
#define SERVICE_IPS         "/camera_streamer/get_host_ip"
#define SERVICE_DISCOVERY   "/camera_directory/discover"
#define TOPIC_CAMERAS       "/camera_directory/cameras"

extern rclcpp::QoS discover_qos; 