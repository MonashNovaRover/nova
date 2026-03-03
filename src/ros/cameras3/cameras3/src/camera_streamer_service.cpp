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
#include <gst/gst.h>

#include <camera_msgs/srv/camera_operation.hpp>
#include <camera_msgs/srv/get_camera_stream_stats.hpp>
#include <camera_msgs/srv/get_ip_list.hpp>
#include <camera_msgs/msg/camera.hpp>
#include <camera_msgs/msg/cameras.hpp>

#include "cameras3/cameras3.hpp"
#include "cameras3/streamer_parameters.hpp"

using namespace std::chrono_literals;
using namespace std::placeholders;

/*
Priority List:
- Implement profiles
  - These should be applied after default but before other parameters
- Pipeline element extra settings
  - E.g look for element factory name like v4l2src then add props 
- Handle extra params and extra meta
  - e.g h264 format option etc
  - hopefully should be easy if pipeline thing works
- gst-launch-1.0 string parameter to pipeline
- ros2 topic pipeline (both ways)
  - this should hopefully be trivial if string parameter is done
- Documentation and code comments (LOWEST PRIORITY LOL)
*/

enum CameraState {STOP = 0, START = 1, PAUSE = 2};

struct CameraProperties
{
  int width;
  int height;
  int framerate; // framerate / 1
  std::string mime;
};

struct PipelineProperties
{
  std::string pipeline_rep;
  bool do_fec;
  bool do_retransmission;
  std::string congestion_control;
  bool show_clock;
  //std::unordered_map<std::string, std::any> extra_meta;
};

struct Pipeline
{
  std::string node;
  GstElement *gst_pipeline;
  CameraProperties *camera_properties;
  PipelineProperties *pipeline_properties;
};



class CameraStreamer : public rclcpp::Node
{
  public: CameraStreamer()
    : Node("camera_streamer", 
      rclcpp::NodeOptions()
        .allow_undeclared_parameters(true)
        .automatically_declare_parameters_from_overrides(true)
    )
  {
    start_service_ = this->create_service<camera_msgs::srv::CameraOperation>(
      SERVICE_START, 
      std::bind(&CameraStreamer::operation_callback, 
        this, _1, _2, CameraState::START)
    );
    stop_service_ = this->create_service<camera_msgs::srv::CameraOperation>(
      SERVICE_STOP, 
      std::bind(&CameraStreamer::operation_callback, 
        this, _1, _2, CameraState::STOP)
    );
    pause_service_ = this->create_service<camera_msgs::srv::CameraOperation>(
      SERVICE_PAUSE, 
      std::bind(&CameraStreamer::operation_callback, 
        this, _1, _2, CameraState::PAUSE)
    );
    stats_service_ = this->create_service<camera_msgs::srv::GetCameraStreamStats>(
      SERVICE_STATS, 
      std::bind(&CameraStreamer::stats_callback, 
        this, _1, _2)
    );
    ips_service_ = this->create_service<camera_msgs::srv::GetIPList>(
      SERVICE_IPS, 
      std::bind(&CameraStreamer::ips_callback, 
        this, _1, _2)
    );

    subscription_ = this->create_subscription<camera_msgs::msg::Cameras>(
      TOPIC_CAMERAS, discover_qos, std::bind(&CameraStreamer::topic_callback, this, _1));
    RCLCPP_INFO(this->get_logger(), "Cameras3 Streamer Running...");

    param_listener = std::make_shared<camera_streamer_service::ParamListener>(get_node_parameters_interface());
    
  }

  rclcpp::Service<camera_msgs::srv::CameraOperation>::SharedPtr start_service_;
  rclcpp::Service<camera_msgs::srv::CameraOperation>::SharedPtr stop_service_;
  rclcpp::Service<camera_msgs::srv::CameraOperation>::SharedPtr pause_service_;
  rclcpp::Service<camera_msgs::srv::GetCameraStreamStats>::SharedPtr stats_service_;
  rclcpp::Service<camera_msgs::srv::GetIPList>::SharedPtr ips_service_;
  rclcpp::Subscription<camera_msgs::msg::Cameras>::SharedPtr subscription_;
  std::shared_ptr<camera_streamer_service::ParamListener> param_listener;
  std::unordered_map<std::string, Pipeline*> pipelines;

  private: void get_properties(std::string serial, Pipeline* pipeline)
  {
    camera_streamer_service::Params params = param_listener->get_params();
    std::map<std::string, rclcpp::Parameter> serial_params;

    // set defaults
    pipeline->camera_properties->width = params.defaults.camera_properties.width;
    pipeline->camera_properties->height = params.defaults.camera_properties.height;
    pipeline->camera_properties->framerate = params.defaults.camera_properties.framerate;
    pipeline->camera_properties->mime = params.defaults.camera_properties.mime;
    pipeline->pipeline_properties->congestion_control = params.defaults.pipeline_properties.congestion_control;
    pipeline->pipeline_properties->do_fec = params.defaults.pipeline_properties.do_fec;
    pipeline->pipeline_properties->do_retransmission = params.defaults.pipeline_properties.do_retransmission;
    pipeline->pipeline_properties->show_clock = params.defaults.pipeline_properties.show_clock;

    // override any defaults with params
    this->get_parameters("cameras." + serial, serial_params);
    RCLCPP_INFO(this->get_logger(), "get props for %s", serial.c_str());
    if (!serial_params.empty()) {
      std::map<std::string, rclcpp::Parameter> camera_params;
      this->get_parameters("cameras." + serial + ".camera_properties", camera_params);
      if (!camera_params.empty()) {
        pipeline->camera_properties->width = camera_params.find("width") != camera_params.end() ? camera_params["width"].as_int() : pipeline->camera_properties->width;
        pipeline->camera_properties->height = camera_params.find("height") != camera_params.end() ? camera_params["height"].as_int() : pipeline->camera_properties->height;
        pipeline->camera_properties->framerate = camera_params.find("framerate") != camera_params.end() ? camera_params["framerate"].as_int() : pipeline->camera_properties->framerate;
        pipeline->camera_properties->mime = camera_params.find("mime") != camera_params.end() ? camera_params["mime"].as_string() : pipeline->camera_properties->mime;
      }
      std::map<std::string, rclcpp::Parameter> pipeline_params;
      this->get_parameters("cameras." + serial + ".pipeline_properties", pipeline_params);
      if (!camera_params.empty()) {
        pipeline->pipeline_properties->congestion_control = pipeline_params.find("congestion_control") != pipeline_params.end() ? pipeline_params["congestion_control"].as_string() : pipeline->pipeline_properties->congestion_control;
        pipeline->pipeline_properties->do_fec = pipeline_params.find("do_fec") != pipeline_params.end() ? pipeline_params["do_fec"].as_bool() : pipeline->pipeline_properties->do_fec;
        pipeline->pipeline_properties->do_retransmission = pipeline_params.find("do_retransmission") != pipeline_params.end() ? pipeline_params["do_retransmission"].as_bool() : pipeline->pipeline_properties->do_retransmission;
        pipeline->pipeline_properties->show_clock = pipeline_params.find("show_clock") != pipeline_params.end() ? pipeline_params["show_clock"].as_bool() : pipeline->pipeline_properties->show_clock;
        // i cbb implementing extra_meta we don't use it anyway
      }
      // RCLCPP_INFO(this->get_logger(), "params, %d, %d, %d, %s, %s, %d, %d, %d", 
      //   pipeline->camera_properties->width, pipeline->camera_properties->height, 
      //   pipeline->camera_properties->framerate, pipeline->camera_properties->mime.c_str(),
      //   pipeline->pipeline_properties->congestion_control.c_str(), pipeline->pipeline_properties->do_fec,
      //   pipeline->pipeline_properties->do_retransmission, pipeline->pipeline_properties->show_clock
      // );
    }
  }

  private: void topic_callback(const camera_msgs::msg::Cameras msg)
  {
    for (camera_msgs::msg::Camera camera : msg.cameras) {
      if (this->pipelines.find(camera.serial) != pipelines.end()) {
        this->pipelines[camera.serial]->node = camera.node;
      } else {
        Pipeline* pipeline = new Pipeline();
        pipeline->node = camera.node;
        pipeline->gst_pipeline = nullptr;
        pipeline->camera_properties = new CameraProperties();
        pipeline->pipeline_properties = new PipelineProperties();
        get_properties(camera.serial, pipeline);
        this->pipelines[camera.serial] = pipeline;
      }
    }
  }

  private: int create_pipeline(std::string serial)
  {
    // currently this just creates a v4l camera to webrtc pipeline
    Pipeline* pipeline = this->pipelines[serial];
    
    pipeline->gst_pipeline = gst_pipeline_new(serial.c_str());
    GstElement* source = gst_element_factory_make("v4l2src", "video-source");
    GstElement* filter = gst_element_factory_make("capsfilter", "filter");
    GstElement* decode = gst_element_factory_make("decodebin", "decoder");
    GstElement* convert = gst_element_factory_make("videoconvert", "converter");
    GstElement* webrtc = gst_element_factory_make("webrtcsink", "webrtc");
    GstElement* clock = pipeline->pipeline_properties->show_clock ? gst_element_factory_make("clockoverlay", "clock") : nullptr;
    
    if (!pipeline->gst_pipeline || !source || !filter || !decode || !convert || !webrtc
        || (pipeline->pipeline_properties->show_clock && !clock) 
      ) {
      RCLCPP_ERROR(this->get_logger(), "Could not create pipeline for %s", serial.c_str());
      return -1;
    }
    RCLCPP_INFO(this->get_logger(), "Starting pipeline for %s with %dx%d@%dfps", serial.c_str(), pipeline->camera_properties->width, pipeline->camera_properties->height, pipeline->camera_properties->framerate);
    g_object_set(source, "device", pipeline->node.c_str(), NULL);
    GstCaps *caps = gst_caps_new_simple(
      pipeline->camera_properties->mime.c_str(),
      "width", G_TYPE_INT, pipeline->camera_properties->width,
      "height", G_TYPE_INT, pipeline->camera_properties->height,
      "framerate", GST_TYPE_FRACTION, pipeline->camera_properties->framerate, 1, NULL);
    g_object_set(filter, "caps", caps, NULL);
    gst_caps_unref(caps);
    GstStructure *meta = gst_structure_new("meta", "serial", G_TYPE_STRING, serial.c_str(), NULL); 
    g_object_set(webrtc,
      "do-fec", pipeline->pipeline_properties->do_fec,
      "do-retransmission", pipeline->pipeline_properties->do_retransmission,
      "congestion-control", (
        pipeline->pipeline_properties->congestion_control == "disabled" ? 0 :
        pipeline->pipeline_properties->congestion_control == "homegrown" ? 1 :
        pipeline->pipeline_properties->congestion_control == "gcc" ? 2 : -1),
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

    bool ret = true;

    ret = gst_element_link(source, filter) ? ret : false;
    ret = gst_element_link(filter, decode) ? ret : false;

    if (pipeline->pipeline_properties->show_clock) {
      gst_bin_add(GST_BIN(pipeline->gst_pipeline), clock);
      ret = gst_element_link(convert, clock) ? ret : false;
      ret = gst_element_link(clock, webrtc) ? ret : false;
    } else {
      ret = gst_element_link(convert, webrtc) ? ret : false;
    }

    if (!ret) {
      RCLCPP_ERROR(this->get_logger(), "Could not link elements of pipeline for %s", serial.c_str());
      return -1;
    }

    return 0;
  }

  private: void operation_callback(
    const std::shared_ptr<camera_msgs::srv::CameraOperation::Request> request,
    std::shared_ptr<camera_msgs::srv::CameraOperation::Response> response,
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
            Pipeline* pipeline = pipelines[serial];
            RCLCPP_INFO(this->get_logger(), "Creating and playing %s", serial.c_str());
            get_properties(serial, pipeline);
            ret = create_pipeline(serial);
            if (ret != 0) return;
            ret = gst_element_set_state(pipeline->gst_pipeline, GST_STATE_PLAYING);
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
    const std::shared_ptr<camera_msgs::srv::GetCameraStreamStats::Request>,
    std::shared_ptr<camera_msgs::srv::GetCameraStreamStats::Response> response)  
  {
    /* TODO: convert this python into c++ (its not used for now anyway)
      result = {
        serial: self._camera_bins[serial].webrtc_stats
        for serial in (request.serials if request.serials else self._camera_bins.keys())
        if serial in self._camera_bins
      }
      response.result_json = json.dumps(result, indent=None if request.indent == 0 else request.indent)
    */
    response->result_json = "NOT IMPLEMENTED";
  }

  private: void ips_callback(
    const std::shared_ptr<camera_msgs::srv::GetIPList::Request>,
    std::shared_ptr<camera_msgs::srv::GetIPList::Response> response)  
  {
    /* TODO: Convert this python to c++ (its not used for now anyway)
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