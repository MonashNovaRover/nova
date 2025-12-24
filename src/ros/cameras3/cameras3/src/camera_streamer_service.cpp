#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <any>

#include "rclcpp/rclcpp.hpp"
#include "std_srvs/srv/empty.hpp"
#include <cameras3_msgs/srv/camera_operation.hpp>
#include <cameras3_msgs/srv/get_camera_stream_stats.hpp>
#include <cameras3_msgs/srv/get_ip_list.hpp>
#include <cameras3_msgs/msg/camera.hpp>
#include <cameras3_msgs/msg/cameras.hpp>

#include <gst/gst.h>

#include "cameras3/cameras3.hpp"

using namespace std::chrono_literals;
using namespace std::placeholders;

/*
CLONE CAMERAS2 Streamer Service TODO:
- Convert stats and ip services from python to c++
- ROS2 Topic pipeline from parameters

New Features Streamer Service TODO:
- Configurable parameters using "generate_parameter_library"
- gst-launch-1.0 string parameter to pipeline
*/

enum CameraState {STOP = 0, START = 1, PAUSE = 2};

struct CameraProperties
{
  int width;
  int height;
  int framerate; // framerate / 1
  std::string mime;
  std::unordered_map<std::string, std::any> meta;
};

struct PipelineProperties
{
  std::string pipeline_rep;
  int do_fec;
  int do_retransmission;
  int congestion_control;
  bool show_clock;
};

struct Pipeline
{
  std::string node;
  GstElement *gst_pipeline;
  CameraProperties *camera_props;
  PipelineProperties *pipeline_properties;
};



class CameraStreamer : public rclcpp::Node
{
  public: CameraStreamer() : Node("camera_streamer")
  {
    start_service_ = this->create_service<cameras3_msgs::srv::CameraOperation>(
      SERVICE_START, 
      std::bind(&CameraStreamer::operation_callback, 
        this, _1, _2, CameraState::START)
    );
    stop_service_ = this->create_service<cameras3_msgs::srv::CameraOperation>(
      SERVICE_STOP, 
      std::bind(&CameraStreamer::operation_callback, 
        this, _1, _2, CameraState::STOP)
    );
    pause_service_ = this->create_service<cameras3_msgs::srv::CameraOperation>(
      SERVICE_PAUSE, 
      std::bind(&CameraStreamer::operation_callback, 
        this, _1, _2, CameraState::PAUSE)
    );
    stats_service_ = this->create_service<cameras3_msgs::srv::GetCameraStreamStats>(
      SERVICE_STATS, 
      std::bind(&CameraStreamer::stats_callback, 
        this, _1, _2)
    );
    ips_service_ = this->create_service<cameras3_msgs::srv::GetIPList>(
      SERVICE_IPS, 
      std::bind(&CameraStreamer::ips_callback, 
        this, _1, _2)
    );

    subscription_ = this->create_subscription<cameras3_msgs::msg::Cameras>(
      TOPIC_CAMERAS, discover_qos, std::bind(&CameraStreamer::topic_callback, this, _1));
    RCLCPP_INFO(this->get_logger(), "Cameras3 Streamer Running...");
  }

  rclcpp::Service<cameras3_msgs::srv::CameraOperation>::SharedPtr start_service_;
  rclcpp::Service<cameras3_msgs::srv::CameraOperation>::SharedPtr stop_service_;
  rclcpp::Service<cameras3_msgs::srv::CameraOperation>::SharedPtr pause_service_;
  rclcpp::Service<cameras3_msgs::srv::GetCameraStreamStats>::SharedPtr stats_service_;
  rclcpp::Service<cameras3_msgs::srv::GetIPList>::SharedPtr ips_service_;
  rclcpp::Subscription<cameras3_msgs::msg::Cameras>::SharedPtr subscription_;
  std::unordered_map<std::string, Pipeline*> pipelines;

  private: void get_properties(std::string serial, Pipeline* pipeline)
  {
    // get properties from yaml
    // defaults
    CameraProperties* camera_props = new CameraProperties();
    camera_props->width = 640;
    camera_props->height = 480;
    camera_props->framerate = 10;
    camera_props->mime = "image/jpeg";
    pipeline->camera_props = camera_props;
    PipelineProperties* pipeline_properties = new PipelineProperties();
    pipeline_properties->congestion_control = 2;
    pipeline_properties->do_fec = 1;
    pipeline_properties->do_retransmission = 1;
    pipeline_properties->show_clock = false;
    pipeline->pipeline_properties = pipeline_properties;
  }

  private: void topic_callback(const cameras3_msgs::msg::Cameras msg)
  {
    for (cameras3_msgs::msg::Camera camera : msg.cameras) {
      if (this->pipelines.find(camera.serial) != pipelines.end()) {
        this->pipelines[camera.serial]->node = camera.node;
      } else {
        Pipeline* pipeline = new Pipeline();
        pipeline->node = camera.node;
        pipeline->gst_pipeline = nullptr;
        get_properties(camera.serial, pipeline);
        this->pipelines[camera.serial] = pipeline;
      }
    }
  }

  private: int create_pipeline(std::string serial)
  {
    Pipeline* pipeline = this->pipelines[serial];
    
    pipeline->gst_pipeline = gst_pipeline_new(serial.c_str());
    GstElement* source = gst_element_factory_make("v4l2src", "video-source");
    GstElement* filter = gst_element_factory_make("capsfilter", "filter");
    GstElement* decode = gst_element_factory_make("decodebin", "decoder");
    GstElement* convert = gst_element_factory_make("videoconvert", "converter");
    GstElement* webrtc = gst_element_factory_make("webrtcsink", "webrtc");
    
    if (!pipeline->gst_pipeline || !source || !filter || !decode || !convert || !webrtc) {
      RCLCPP_ERROR(this->get_logger(), "Could not create pipeline for %s", serial.c_str());
      return -1;
    }
    RCLCPP_INFO(this->get_logger(), "Starting pipeline with %dx%d@%dfps", pipeline->camera_props->width, pipeline->camera_props->height, pipeline->camera_props->framerate);
    g_object_set(source, "device", pipeline->node.c_str(), NULL);
    GstCaps *caps = gst_caps_new_simple(
      pipeline->camera_props->mime.c_str(),
      "width", G_TYPE_INT, pipeline->camera_props->width,
      "height", G_TYPE_INT, pipeline->camera_props->height,
      "framerate", GST_TYPE_FRACTION, pipeline->camera_props->framerate, 1, NULL);
    g_object_set(filter, "caps", caps, NULL);
    gst_caps_unref(caps);
    GstStructure *meta = gst_structure_new("meta", "serial", G_TYPE_STRING, serial.c_str(), NULL); 
    g_object_set(webrtc,
      "do-fec", pipeline->pipeline_properties->do_fec,
      "do-retransmission", pipeline->pipeline_properties->do_retransmission,
      "congestion-control", pipeline->pipeline_properties->congestion_control,
      "meta", meta, 
      NULL);
    gst_structure_free(meta);
    
    gst_bin_add_many(GST_BIN(pipeline->gst_pipeline), source, filter, decode, convert, webrtc, NULL);

    g_signal_connect(decode, "pad-added", G_CALLBACK(+[](GstElement* /*decode*/, GstPad* new_pad, gpointer user_data) {
      GstElement* convert = static_cast<GstElement*>(user_data);
      GstPad* sink_pad = gst_element_get_static_pad(convert, "sink");
      if (sink_pad && !gst_pad_is_linked(sink_pad)) {
          gst_pad_link(new_pad, sink_pad);
      }
      if (sink_pad) gst_object_unref(sink_pad);
    }), convert);

    if(!(
        gst_element_link(source, filter) &&
        gst_element_link(filter, decode) &&
        gst_element_link(convert, webrtc)
      )) {
        RCLCPP_ERROR(this->get_logger(), "Could not link elements of pipeline for %s", serial.c_str());
        return -1;
    }

    return 0;
  }

  private: void operation_callback(
    const std::shared_ptr<cameras3_msgs::srv::CameraOperation::Request> request,
    std::shared_ptr<cameras3_msgs::srv::CameraOperation::Response> response,
    CameraState state)  
  {
    int ret;
    response->success = false;
    switch (state) {
      case CameraState::START:
        for (std::string serial : request->serials) {
          if (this->pipelines.find(serial) != pipelines.end() && this->pipelines[serial]->gst_pipeline != nullptr) {
            // gstreamer play pipeline if paused
            RCLCPP_INFO(this->get_logger(), "Resuming %s", serial.c_str());
            Pipeline* pipeline = pipelines[serial];
            gst_element_set_state(pipeline->gst_pipeline, GST_STATE_PLAYING);
          }
          else {
            RCLCPP_INFO(this->get_logger(), "Creating and playing %s", serial.c_str());
            ret = create_pipeline(serial);
            if (ret != 0) return;
            ret = gst_element_set_state(this->pipelines[serial]->gst_pipeline, GST_STATE_PLAYING);
            if (ret != 0) return;
            response->success = true;
          }
        }
        break;
      case CameraState::STOP:
        for (std::string serial : request->serials) {
          if (this->pipelines.find(serial) != pipelines.end() && this->pipelines[serial]->gst_pipeline != nullptr) {
            Pipeline* pipeline = pipelines[serial];
            gst_element_set_state(pipeline->gst_pipeline, GST_STATE_NULL);
            gst_object_unref(pipeline->gst_pipeline);
            RCLCPP_INFO(this->get_logger(), "Stopping %s", serial.c_str());
          }
        }
        break;
      case CameraState::PAUSE:
        for (std::string serial : request->serials) {
          if (this->pipelines.find(serial) != pipelines.end() && this->pipelines[serial]->gst_pipeline != nullptr) {
            Pipeline* pipeline = pipelines[serial];
            gst_element_set_state(pipeline->gst_pipeline, GST_STATE_PAUSED);
            RCLCPP_INFO(this->get_logger(), "Pausing %s", serial.c_str());
          }
        }
        break;
    }
  }

  private: void stats_callback(
    const std::shared_ptr<cameras3_msgs::srv::GetCameraStreamStats::Request> request,
    std::shared_ptr<cameras3_msgs::srv::GetCameraStreamStats::Response> response)  
  {
    /* TODO: convert this python into c++
      result = {
        serial: self._camera_bins[serial].webrtc_stats
        for serial in (request.serials if request.serials else self._camera_bins.keys())
        if serial in self._camera_bins
      }
      response.result_json = json.dumps(result, indent=None if request.indent == 0 else request.indent)
    */
    response->result_json = "";
  }

  private: void ips_callback(
    const std::shared_ptr<cameras3_msgs::srv::GetIPList::Request> request,
    std::shared_ptr<cameras3_msgs::srv::GetIPList::Response> response)  
  {
    /* TODO: Convert this python to c++
      if_addrs = psutil.net_if_addrs()
      if_stats = psutil.net_if_stats()
      addresses = {
        interface: address
        for interface, addresses in if_addrs.items()
        if (
            address := next(
                (address.address for address in addresses if address.family == AddressFamily.AF_INET),
                None,
            )
        )
        is not None
      }

      def key(entry: tuple[str, str]) -> int:
        interface = entry[0]
        address = entry[1]
        flags = set(if_stats[interface].flags.split(","))
        if "loopback" in flags:  # Local
            return 0
        elif address.startswith("192.168.1"):  # Nova radios
            return 1
        elif address.startswith("192.168.0"):  # Nova Wi-Fi
            return 2
        else:  # Other
            return 3

      response.ips = [address for interface, address in sorted(addresses.items(), key=key)]
      return response
    */
   std::vector<std::string> ips;
   response->ips = ips;
  }
};

int main(int argc, char * argv[])
{
  gst_init(&argc, &argv);
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<CameraStreamer>());
  rclcpp::shutdown();
  return 0;
}