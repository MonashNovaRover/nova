#include <nova_costmap_2d/height_mapper_layer.hpp>

namespace nova_costmap_2d
{
void HeightMapperLayer::onInitialize()
{
  bool track_unknown_space;
  double transform_tolerance;

  // The topics that we'll subscribe to from the parameter server
  std::string topics_string;

  declareParameter("enabled", rclcpp::ParameterValue(true));
  declareParameter("footprint_clearing_enabled", rclcpp::ParameterValue(true));
  declareParameter("min_obstacle_height", rclcpp::ParameterValue(0.0));
  declareParameter("max_obstacle_height", rclcpp::ParameterValue(2.0));
  declareParameter("combination_method", rclcpp::ParameterValue(1));
  declareParameter("observation_sources", rclcpp::ParameterValue(std::string("")));
  declareParameter("max_safe_val", rclcpp::ParameterValue(16.0));  // Approximately represents total height diff in a 10x10cm area
  declareParameter("resolution_ratio", rclcpp::ParameterValue(4));  // Ratio between resolution of mini-heightmaps and final costmap
  declareParameter("min_plane_density", rclcpp::ParameterValue(0.3));  // Ratio between resolution of mini-heightmaps and final costmap

  auto node = node_.lock();
  if (!node) {
    throw std::runtime_error{"Failed to lock node"};
  }

  node->get_parameter(name_ + "." + "enabled", enabled_);
  node->get_parameter(name_ + "." + "footprint_clearing_enabled", footprint_clearing_enabled_);
  node->get_parameter(name_ + "." + "min_obstacle_height", min_obstacle_height_);
  node->get_parameter(name_ + "." + "max_obstacle_height", max_obstacle_height_);
  node->get_parameter("track_unknown_space", track_unknown_space);
  node->get_parameter("transform_tolerance", transform_tolerance);
  node->get_parameter(name_ + "." + "observation_sources", topics_string);
  node->get_parameter(name_ + "." + "max_safe_val", max_safe_val_);
  node->get_parameter(name_ + "." + "resolution_ratio", resolution_ratio_);
  node->get_parameter(name_ + "." + "min_plane_density", min_plane_density_);

  int combination_method_param{};
  node->get_parameter(name_ + "." + "combination_method", combination_method_param);
  combination_method_ = combination_method_from_int(combination_method_param);

  dyn_params_handler_ = node->add_on_set_parameters_callback(
    std::bind(
      &HeightMapperLayer::dynamicParametersCallback,
      this,
      std::placeholders::_1));

  RCLCPP_INFO(
    logger_,
    "Subscribed to Topics: %s", topics_string.c_str());

  rolling_window_ = layered_costmap_->isRolling();

  if (track_unknown_space) {
    default_value_ = nav2_costmap_2d::NO_INFORMATION;
  } else {
    default_value_ = nav2_costmap_2d::FREE_SPACE;
  }

  HeightMapperLayer::matchSize();
  current_ = true;
  was_reset_ = false;

  global_frame_ = layered_costmap_->getGlobalFrameID();

  auto sub_opt = rclcpp::SubscriptionOptions();
  sub_opt.callback_group = callback_group_;

  // now we need to split the topics based on whitespace which we can use a stringstream for
  std::stringstream ss(topics_string);

  std::string source;
  while (ss >> source) {
    // get the parameters for the specific topic
    double observation_keep_time, expected_update_rate, min_obstacle_height, max_obstacle_height;
    std::string topic, sensor_frame, data_type;
    bool inf_is_valid, clearing, marking;

    declareParameter(source + "." + "topic", rclcpp::ParameterValue(source));
    declareParameter(source + "." + "sensor_frame", rclcpp::ParameterValue(std::string("")));
    declareParameter(source + "." + "observation_persistence", rclcpp::ParameterValue(0.0));
    declareParameter(source + "." + "expected_update_rate", rclcpp::ParameterValue(0.0));
    declareParameter(source + "." + "data_type", rclcpp::ParameterValue(std::string("LaserScan")));
    declareParameter(source + "." + "min_obstacle_height", rclcpp::ParameterValue(0.0));
    declareParameter(source + "." + "max_obstacle_height", rclcpp::ParameterValue(0.0));
    declareParameter(source + "." + "inf_is_valid", rclcpp::ParameterValue(false));
    declareParameter(source + "." + "marking", rclcpp::ParameterValue(true));
    declareParameter(source + "." + "clearing", rclcpp::ParameterValue(false));
    declareParameter(source + "." + "obstacle_max_range", rclcpp::ParameterValue(2.5));
    declareParameter(source + "." + "obstacle_min_range", rclcpp::ParameterValue(0.0));
    declareParameter(source + "." + "raytrace_max_range", rclcpp::ParameterValue(3.0));
    declareParameter(source + "." + "raytrace_min_range", rclcpp::ParameterValue(0.0));

    node->get_parameter(name_ + "." + source + "." + "topic", topic);
    node->get_parameter(name_ + "." + source + "." + "sensor_frame", sensor_frame);
    node->get_parameter(
      name_ + "." + source + "." + "observation_persistence",
      observation_keep_time);
    node->get_parameter(
      name_ + "." + source + "." + "expected_update_rate",
      expected_update_rate);
    node->get_parameter(name_ + "." + source + "." + "data_type", data_type);
    node->get_parameter(name_ + "." + source + "." + "min_obstacle_height", min_obstacle_height);
    node->get_parameter(name_ + "." + source + "." + "max_obstacle_height", max_obstacle_height);
    node->get_parameter(name_ + "." + source + "." + "inf_is_valid", inf_is_valid);
    node->get_parameter(name_ + "." + source + "." + "marking", marking);
    node->get_parameter(name_ + "." + source + "." + "clearing", clearing);

    if (!(data_type == "PointCloud2" || data_type == "LaserScan")) {
      RCLCPP_FATAL(
        logger_,
        "Only topics that use point cloud2s or laser scans are currently supported");
      throw std::runtime_error(
              "Only topics that use point cloud2s or laser scans are currently supported");
    }

    // get the obstacle range for the sensor
    double obstacle_max_range, obstacle_min_range;
    node->get_parameter(name_ + "." + source + "." + "obstacle_max_range", obstacle_max_range);
    node->get_parameter(name_ + "." + source + "." + "obstacle_min_range", obstacle_min_range);

    // get the raytrace ranges for the sensor
    double raytrace_max_range, raytrace_min_range;
    node->get_parameter(name_ + "." + source + "." + "raytrace_min_range", raytrace_min_range);
    node->get_parameter(name_ + "." + source + "." + "raytrace_max_range", raytrace_max_range);


    RCLCPP_DEBUG(
      logger_,
      "Creating an observation buffer for source %s, topic %s, frame %s",
      source.c_str(), topic.c_str(),
      sensor_frame.c_str());

    // create an observation buffer
    observation_buffers_.push_back(
      std::shared_ptr<nav2_costmap_2d::ObservationBuffer
      >(
        new nav2_costmap_2d::ObservationBuffer(
          node, topic, observation_keep_time, expected_update_rate,
          min_obstacle_height,
          max_obstacle_height, obstacle_max_range, obstacle_min_range, raytrace_max_range,
          raytrace_min_range, *tf_,
          global_frame_,
          sensor_frame, tf2::durationFromSec(transform_tolerance))));

    // check if we'll add this buffer to our marking observation buffers
    if (marking) {
      marking_buffers_.push_back(observation_buffers_.back());
    }

    // check if we'll also add this buffer to our clearing observation buffers
    if (clearing) {
      clearing_buffers_.push_back(observation_buffers_.back());
    }

    RCLCPP_DEBUG(
      logger_,
      "Created an observation buffer for source %s, topic %s, global frame: %s, "
      "expected update rate: %.2f, observation persistence: %.2f",
      source.c_str(), topic.c_str(),
      global_frame_.c_str(), expected_update_rate, observation_keep_time);

    rmw_qos_profile_t custom_qos_profile = rmw_qos_profile_sensor_data;
    custom_qos_profile.depth = 50;

    // create a callback for the topic
    if (data_type == "LaserScan") {
      auto sub = std::make_shared<message_filters::Subscriber<sensor_msgs::msg::LaserScan,
          rclcpp_lifecycle::LifecycleNode>>(node, topic, custom_qos_profile, sub_opt);
      sub->unsubscribe();

      auto filter = std::make_shared<tf2_ros::MessageFilter<sensor_msgs::msg::LaserScan>>(
        *sub, *tf_, global_frame_, 50,
        node->get_node_logging_interface(),
        node->get_node_clock_interface(),
        tf2::durationFromSec(transform_tolerance));

      if (inf_is_valid) {
        filter->registerCallback(
          std::bind(
            &HeightMapperLayer::laserScanValidInfCallback, this, std::placeholders::_1,
            observation_buffers_.back()));

      } else {
        filter->registerCallback(
          std::bind(
            &HeightMapperLayer::laserScanCallback, this, std::placeholders::_1,
            observation_buffers_.back()));
      }

      observation_subscribers_.push_back(sub);

      observation_notifiers_.push_back(filter);
      observation_notifiers_.back()->setTolerance(rclcpp::Duration::from_seconds(0.05));

    } else {
      auto sub = std::make_shared<message_filters::Subscriber<sensor_msgs::msg::PointCloud2,
          rclcpp_lifecycle::LifecycleNode>>(node, topic, custom_qos_profile, sub_opt);
      sub->unsubscribe();

      if (inf_is_valid) {
        RCLCPP_WARN(
          logger_,
          "obstacle_layer: inf_is_valid option is not applicable to PointCloud observations.");
      }

      auto filter = std::make_shared<tf2_ros::MessageFilter<sensor_msgs::msg::PointCloud2>>(
        *sub, *tf_, global_frame_, 50,
        node->get_node_logging_interface(),
        node->get_node_clock_interface(),
        tf2::durationFromSec(transform_tolerance));

      filter->registerCallback(
        std::bind(
          &HeightMapperLayer::pointCloud2Callback, this, std::placeholders::_1,
          observation_buffers_.back()));

      observation_subscribers_.push_back(sub);
      observation_notifiers_.push_back(filter);
    }

    if (sensor_frame != "") {
      std::vector<std::string> target_frames;
      target_frames.push_back(global_frame_);
      target_frames.push_back(sensor_frame);
      observation_notifiers_.back()->setTargetFrames(target_frames);
    }
  }
}

HeightMapperLayer::~HeightMapperLayer()
{
  dyn_params_handler_.reset();
	for (auto & notifier : observation_notifiers_) {
		notifier.reset();
	}
}

bool HeightMapperLayer::worldToIntermediateMap(double wx, double wy, uint32_t & mx, uint32_t & my) const
{
  if (wx < origin_x_ || wy < origin_y_) {
    return false;
  }

  mx = static_cast<uint32_t>(resolution_ratio_ * (wx - origin_x_) / resolution_);
  my = static_cast<uint32_t>(resolution_ratio_ * (wy - origin_y_) / resolution_);

  if (mx < resolution_ratio_ * size_x_ && my < resolution_ratio_ * size_y_) {
    return true;
  }
  return false;
}

bool HeightMapperLayer::getIntermediateResolution(double & res) const
{
  if (resolution_ratio_ > 0) {
      res = resolution_ / resolution_ratio_;
      return true;
  } else {
    return false;
  }
}

void
HeightMapperLayer::updateBounds(
  double robot_x, double robot_y, double robot_yaw, double * min_x,
  double * min_y, double * max_x, double * max_y)
{
  std::lock_guard<Costmap2D::mutex_t> guard(*getMutex());
  if (rolling_window_) {
    updateOrigin(robot_x - getSizeInMetersX() / 2, robot_y - getSizeInMetersY() / 2);
  }
  if (!enabled_) {
    return;
  }
  useExtraBounds(min_x, min_y, max_x, max_y);

  bool current = true;
  std::vector<nav2_costmap_2d::Observation> observations, clearing_observations;

	if (clearing_observations.size() > 0) RCLCPP_WARN(logger_, "Raytracing not implemented for HeightMapperLayer");

  // get the marking observations
  current = current && getMarkingObservations(observations);
  current_ = current;

  double intermediate_resolution;

  if (!getIntermediateResolution(intermediate_resolution)) {
    RCLCPP_ERROR(logger_, "Failed to calculate intermediate resolution! Check resolution_ratio parameter > 0");
    return;
  }

  const uint32_t xs = getSizeInCellsX();
  const uint32_t ys = getSizeInCellsY();
  const uint32_t XS = xs * resolution_ratio_;
  const uint32_t YS = ys * resolution_ratio_;

  const float C_NEG_INF = -std::numeric_limits<float>::infinity();
  const float C_INF = std::numeric_limits<float>::infinity();

	cv::Mat top_height_map(cv::Size(XS, YS), CV_32FC1, cv::Scalar(C_NEG_INF));
	cv::Mat bottom_height_map(cv::Size(XS, YS), CV_32FC1, cv::Scalar(C_INF));
	std::vector<std::vector<bool>> has_data_map(xs, std::vector<bool>(ys));

  for (std::vector<nav2_costmap_2d::Observation>::const_iterator it = observations.begin(); it != observations.end(); ++it)
  {
    const nav2_costmap_2d::Observation & obs = *it;

    const sensor_msgs::msg::PointCloud2 & cloud = *(obs.cloud_);

    double sq_obstacle_max_range = obs.obstacle_max_range_ * obs.obstacle_max_range_;
    double sq_obstacle_min_range = obs.obstacle_min_range_ * obs.obstacle_min_range_;

    sensor_msgs::PointCloud2ConstIterator<float> iter_x(cloud, "x");
    sensor_msgs::PointCloud2ConstIterator<float> iter_y(cloud, "y");
    sensor_msgs::PointCloud2ConstIterator<float> iter_z(cloud, "z");

    for (; iter_x != iter_x.end(); ++iter_x, ++iter_y, ++iter_z) {
      double px = *iter_x, py = *iter_y, pz = *iter_z;

      // if the obstacle is too low, we won't add it
      if (pz < min_obstacle_height_) {
        RCLCPP_DEBUG(logger_, "The point is too low");
        continue;
      }

      // if the obstacle is too high or too far away from the robot we won't add it
      if (pz > max_obstacle_height_) {
        RCLCPP_DEBUG(logger_, "The point is too high");
        continue;
      }

			double dx = px - obs.origin_.x, dy = py - obs.origin_.y, dz = pz - obs.origin_.z;

      // compute the squared distance from the hitpoint to the pointcloud's origin
      double sq_dist = dx * dx + dy * dy + dz * dz;

      // if the point is far enough away... we won't consider it
      if (sq_dist >= sq_obstacle_max_range) {
        RCLCPP_DEBUG(logger_, "The point is too far away");
        continue;
      }

      // if the point is too close, do not conisder it
      if (sq_dist < sq_obstacle_min_range) {
        RCLCPP_DEBUG(logger_, "The point is too close");
        continue;
      }

      // now we need to compute the map coordinates for the observation
      uint32_t mx, my;
      if (!worldToIntermediateMap(px, py, mx, my)) {
        RCLCPP_DEBUG(logger_, "Computing intermediate map coords failed");
        continue;
      }

      uint32_t seen_mx, seen_my;
      if (!worldToMap(px, py, seen_mx, seen_my)) {
        RCLCPP_DEBUG(logger_, "Computing map coords failed");
        continue;
      }
      // Update that we have seen this point
      has_data_map.at(seen_mx).at(seen_my) = true;

      float mz = static_cast<float>(pz);
      
			if (mz > top_height_map.at<float> (mx, my)) {
        top_height_map.at<float> (mx, my) = mz;
      }
			if (mz < bottom_height_map.at<float> (mx, my)) {
        bottom_height_map.at<float> (mx, my) = mz;
      }

      touch(px, py, min_x, min_y, max_x, max_y);
    }
  }
  
	//blurring the top height map so we can compare the heights of adjacent points
	top_height_map = cv::max(top_height_map, shift(top_height_map, -1, 0, C_NEG_INF));
	top_height_map = cv::max(top_height_map, shift(top_height_map, 1, 0, C_NEG_INF));
	top_height_map = cv::max(top_height_map, shift(top_height_map, 0, -1, C_NEG_INF));
	top_height_map = cv::max(top_height_map, shift(top_height_map, 0, 1, C_NEG_INF));
	// blurring the bottom height map so all obstacles are at least 2 pixels wide.
	bottom_height_map = cv::min(bottom_height_map, shift(bottom_height_map, -1, 0, C_INF));
	bottom_height_map = cv::min(bottom_height_map, shift(bottom_height_map, 1, 0, C_INF));
	bottom_height_map = cv::min(bottom_height_map, shift(bottom_height_map, 0, -1, C_INF));
	bottom_height_map = cv::min(bottom_height_map, shift(bottom_height_map, 0, 1, C_INF));

	cv::Mat diff; 

	cv::subtract(top_height_map, bottom_height_map, diff);

	for (size_t mx = 0; mx < getSizeInCellsX(); ++mx){
		for (size_t my = 0; my < getSizeInCellsY(); ++my){
			if (!has_data_map.at(mx).at(my)){
        // No pointcloud points fell in this pixel... so don't update
        continue;
			} 

      // Get sum of this region in the diff map
      float val = 0;
      int num_vals = 0;
      for (size_t i = 0; i < static_cast<size_t>(resolution_ratio_); ++i){
        for (size_t j = 0; j < static_cast<size_t>(resolution_ratio_); ++j){
          float cell_val = diff.at<float>(mx * resolution_ratio_ + i, my * resolution_ratio_ + j);
          if (cell_val > C_NEG_INF) {
            num_vals++;
            val += cell_val;
          }
        }
      }      

      if (num_vals < min_plane_density_ * resolution_ratio_ * resolution_ratio_) continue;
      
      val /= (num_vals * intermediate_resolution);

      size_t index = getIndex(mx, my);
      if (val >= max_safe_val_){
        costmap_[index] = nav2_costmap_2d::LETHAL_OBSTACLE;
      } else {
        costmap_[index] = static_cast<uint8_t>(nav2_costmap_2d::MAX_NON_OBSTACLE * val / max_safe_val_);
      }
		}
	}

  updateFootprint(robot_x, robot_y, robot_yaw, min_x, min_y, max_x, max_y);
}

} // nova_costmap_2d

// Register the macro for this layer
#include "pluginlib/class_list_macros.hpp"
PLUGINLIB_EXPORT_CLASS(nova_costmap_2d::HeightMapperLayer, nav2_costmap_2d::Layer)