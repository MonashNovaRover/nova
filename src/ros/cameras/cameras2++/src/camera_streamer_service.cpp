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

#include "cameras/cameras.hpp"
#include "cameras/pipeline.hpp"

using namespace std::placeholders;


enum CameraState {STOP = 0, START = 1, PAUSE = 2};

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
    RCLCPP_INFO(this->get_logger(), "Cameras2++ Streamer Running...");
  }

  rclcpp::Service<camera_msgs::srv::CameraOperation>::SharedPtr start_service_;
  rclcpp::Service<camera_msgs::srv::CameraOperation>::SharedPtr stop_service_;
  rclcpp::Service<camera_msgs::srv::CameraOperation>::SharedPtr pause_service_;
  rclcpp::Service<camera_msgs::srv::GetCameraStreamStats>::SharedPtr stats_service_;
  rclcpp::Service<camera_msgs::srv::GetIPList>::SharedPtr ips_service_;
  rclcpp::Subscription<camera_msgs::msg::Cameras>::SharedPtr subscription_;
  std::unordered_map<std::string, Pipeline*> pipelines;

  private: void start_pipeline(Pipeline* pipeline){
    // get pipeline properties and use them to create the pipeline
    if (pipeline->pipeline_type == "v4l2webrtc")
    {
      auto props = get_v4l2webrtc_pipeline_properties(this, pipeline->camera);
      pipeline->props = props;
      pipeline->gst_pipeline = v4l2webrtc_pipeline(this, props);
    } else if (pipeline->pipeline_type == "h264direct") {
      auto props = get_h264direct_pipeline_properties(this, pipeline->camera);
      pipeline->props = props;
      pipeline->gst_pipeline = h264direct_pipeline(this, props);
    } 
  }

  private: void topic_callback(const camera_msgs::msg::Cameras msg)
  {
    for (camera_msgs::msg::Camera camera : msg.cameras) {
      if (this->pipelines.find(camera.serial) != pipelines.end()) {
      // update node if it changed
        Pipeline* pipeline = this->pipelines.at(camera.serial);
        if (pipeline->camera->node != camera.node) pipeline->props->node = camera.node;
        // reset camera? now with the change
      } else {
      // otherwise make the pipeline
        Pipeline* pipeline = new Pipeline;
        pipeline->camera = new camera_msgs::msg::Camera;
        pipeline->camera->serial=camera.serial;
        pipeline->camera->node=camera.node;

        std::string pipeline_type;
        this->get_parameter_or<std::string>((std::string(PIPELINE_PREFIX) + "." + camera.serial + ".pipeline_type").c_str(), pipeline_type, "v4l2webrtc");
        pipeline->pipeline_type = pipeline_type;
        bool autostart;
        this->get_parameter_or("autostart", autostart, true);

        // auto start if true
        if (autostart) {
          this->start_pipeline(pipeline) ;
        } else {
          pipeline->gst_pipeline = nullptr;
        }
        RCLCPP_INFO(this->get_logger(), "Creating %s pipeline for %s", pipeline_type.c_str(), camera.serial.c_str());
        this->pipelines[camera.serial] = pipeline;
      }
    }
  }


  private: void operation_callback(
    const std::shared_ptr<camera_msgs::srv::CameraOperation::Request> request,
    std::shared_ptr<camera_msgs::srv::CameraOperation::Response> response,
    CameraState state)  
  {
    response->success = true;
    switch (state) {
      case CameraState::START:
        for (std::string serial : request->serials) {
          if (this->pipelines.find(serial) != pipelines.end()) {
            Pipeline* pipeline = pipelines[serial];
            if (this->pipelines[serial]->gst_pipeline != nullptr) {
            // gstreamer play pipeline if paused
              RCLCPP_INFO(this->get_logger(), "Resuming %s", serial.c_str());
              gst_element_set_state(pipeline->gst_pipeline, GST_STATE_PLAYING);
            } else {
            // start pipeline if the gst bin doesn't exist yet
              RCLCPP_INFO(this->get_logger(), "Starting %s", serial.c_str());
              this->start_pipeline(pipeline);
              gst_element_set_state(pipeline->gst_pipeline, GST_STATE_PLAYING);
            }
          } else {
          // otherwise report error
            RCLCPP_INFO(this->get_logger(), "Issue with pipeline of: %s", serial.c_str());
            response->success = false;
          }
        }
        break;
      case CameraState::STOP:
        for (std::string serial : request->serials) {
          if (this->pipelines.find(serial) != pipelines.end() && this->pipelines[serial]->gst_pipeline != nullptr) {
            Pipeline* pipeline = pipelines[serial];
            gst_element_set_state(pipeline->gst_pipeline, GST_STATE_NULL);
            gst_object_unref(pipeline->gst_pipeline);
            pipeline->gst_pipeline = nullptr;
            RCLCPP_INFO(this->get_logger(), "Stopping %s", serial.c_str());
          } else {
            RCLCPP_INFO(this->get_logger(), "Issue with pipeline of: %s", serial.c_str());
            response->success = false;
          }
        }
        break;
      case CameraState::PAUSE:
        for (std::string serial : request->serials) {
          if (this->pipelines.find(serial) != pipelines.end() && this->pipelines[serial]->gst_pipeline != nullptr) {
            Pipeline* pipeline = pipelines[serial];
            gst_element_set_state(pipeline->gst_pipeline, GST_STATE_PAUSED);
            RCLCPP_INFO(this->get_logger(), "Pausing %s", serial.c_str());
          } else {
            RCLCPP_INFO(this->get_logger(), "Issue with pipeline of: %s", serial.c_str());
            response->success = false;
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
