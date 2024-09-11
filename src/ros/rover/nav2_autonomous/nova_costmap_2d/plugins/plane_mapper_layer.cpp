#include <nova_costmap_2d/plane_mapper_layer.hpp>

namespace nova_costmap_2d
{
void PlaneMapperLayer::onInitialize()
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
  declareParameter("max_safe_inc", rclcpp::ParameterValue(20.0));  // Approximately represents total height diff in a 10x10cm area
  declareParameter("resolution_ratio", rclcpp::ParameterValue(4));  // Ratio between resolution of mini-heightmaps and final costmap
  declareParameter("min_plane_density", rclcpp::ParameterValue(0.3));  // Ratio between resolution of mini-heightmaps and final costmap
  declareParameter("plane_overlap", rclcpp::ParameterValue(2));  // Ratio between resolution of mini-heightmaps and final costmap

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
  node->get_parameter(name_ + "." + "max_safe_inc", max_safe_inc_);
  node->get_parameter(name_ + "." + "resolution_ratio", resolution_ratio_);
  node->get_parameter(name_ + "." + "min_plane_density", min_plane_density_);
  node->get_parameter(name_ + "." + "plane_overlap", plane_overlap_);

  int combination_method_param{};
  node->get_parameter(name_ + "." + "combination_method", combination_method_param);
  combination_method_ = combination_method_from_int(combination_method_param);

  dyn_params_handler_ = node->add_on_set_parameters_callback(
    std::bind(
      &PlaneMapperLayer::dynamicParametersCallback,
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

  PlaneMapperLayer::matchSize();
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
            &PlaneMapperLayer::laserScanValidInfCallback, this, std::placeholders::_1,
            observation_buffers_.back()));

      } else {
        filter->registerCallback(
          std::bind(
            &PlaneMapperLayer::laserScanCallback, this, std::placeholders::_1,
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
          &PlaneMapperLayer::pointCloud2Callback, this, std::placeholders::_1,
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

PlaneMapperLayer::~PlaneMapperLayer()
{
  dyn_params_handler_.reset();
	for (auto & notifier : observation_notifiers_) {
		notifier.reset();
	}
}

bool PlaneMapperLayer::worldToIntermediateMap(double wx, double wy, uint32_t & mx, uint32_t & my) const
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

bool PlaneMapperLayer::getIntermediateResolution(double & res) const
{
  if (resolution_ratio_ > 0) {
      res = resolution_ / resolution_ratio_;
      return true;
  } else {
    return false;
  }
}

void
PlaneMapperLayer::updateBounds(
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

	if (clearing_observations.size() > 0) RCLCPP_WARN(logger_, "Raytracing not implemented for PlaneMapperLayer");

  // get the marking observations
  current = current && getMarkingObservations(observations);
  current_ = current;

  const uint32_t xs = getSizeInCellsX();
  const uint32_t ys = getSizeInCellsY();
  const uint32_t XS = xs * resolution_ratio_;
  const uint32_t YS = ys * resolution_ratio_;

  const float C_NEG_INF = -std::numeric_limits<float>::infinity();

	cv::Mat top_height_map(cv::Size(XS, YS), CV_32FC1, cv::Scalar(C_NEG_INF));
	std::vector<std::vector<bool>> has_data_map(xs, std::vector<bool>(ys));

  /**
   * POPULATE HEIGHT MAP
  */
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

      touch(px, py, min_x, min_y, max_x, max_y);
    }
  }

  /**
   * CALCULATE BEST FIT PLANE INCLINATIONS
  */
  cv::Mat incs(cv::Size(xs, ys), CV_32FC1, cv::Scalar(C_NEG_INF));

  for (uint32_t plane_i = 0; plane_i < xs; plane_i++) {
    for (uint32_t plane_j = 0; plane_j < ys; plane_j++) {
      uint32_t min_x = std::max((uint32_t) 0, plane_i * resolution_ratio_ - plane_overlap_);
      uint32_t min_y = std::max((uint32_t) 0, plane_j * resolution_ratio_ - plane_overlap_);
      uint32_t max_x = std::min(XS - 1, (plane_i + 1) * resolution_ratio_ + plane_overlap_);
      uint32_t max_y = std::min(YS - 1, (plane_j + 1) * resolution_ratio_ + plane_overlap_);
      Eigen::Vector3d point_sum(0, 0, 0);
      std::vector<Eigen::Vector3d> these_pts;
      // Construct 3d points from height map coordinates and heights, and
      // Track their sum
      for (std::size_t i = min_x; i < max_x; i++) {
        for (std::size_t j = min_y; j < max_y; j++) {
          float z = top_height_map.at<float>(i, j);
          float x = i * resolution_ / resolution_ratio_;
          float y = j * resolution_ / resolution_ratio_;
          if (z == C_NEG_INF) continue;
          Eigen::Vector3d p(x, y, z);
          these_pts.push_back(p);
          point_sum = point_sum + p;
        }
      }
      
      int pixels_per_plane = (max_x - min_x) * (max_y - min_y);
      int pixels_in_plane = these_pts.size();

      if (pixels_in_plane <= min_plane_density_ * pixels_per_plane) continue;

      Eigen::Vector3d centroid = point_sum / pixels_in_plane;

      double xx=0.0, yy=0.0, zz=0.0, xy=0.0, xz=0.0, yz=0.0;

      bool collinear = true;
      Eigen::Vector3d p0 = these_pts[0];
      Eigen::Vector3d line_dir = (these_pts[1] - p0).normalized();

      // Do plane fitting on plane points
      for (Eigen::Vector3d p : these_pts) {
        Eigen::Vector3d line_vec = p - p0;
        collinear = collinear && (line_dir.cross(line_vec).norm() <= 0.01);
        Eigen::Vector3d r = p - centroid;
        xx += r.x()*r.x();
        xy += r.x()*r.y();
        xz += r.x()*r.z();
        yy += r.y()*r.y();
        yz += r.y()*r.z();
        zz += r.z()*r.z(); 
      }
      // Check for degenerate cases
      if (collinear) continue;

      xx /= pixels_in_plane;
      xy /= pixels_in_plane;
      xz /= pixels_in_plane;
      yy /= pixels_in_plane;
      yz /= pixels_in_plane;
      zz /= pixels_in_plane;

      Eigen::Vector3d weighted_dir, axis_dir;
      double weight;

      double det_x = yy*zz - yz*yz;
      double det_y = xx*zz - xz*xz;
      double det_z = xx*yy - xy*xy;

      // linear regression in x direction
      axis_dir = Eigen::Vector3d(det_x, xz*yz - xy*zz, xy*yz - xz*yy);
      weight = det_x * det_x;

      weighted_dir = axis_dir * weight;

      // linear regression in y direction
      axis_dir = Eigen::Vector3d(xz*yz - xy*zz, det_y, xy*xz - yz*xx);
      weight = det_y * det_y;

      if (weighted_dir.dot(axis_dir) < 0.0) weight = -weight;

      weighted_dir = weighted_dir + axis_dir * weight;

      // linear regression in z direction
      axis_dir = Eigen::Vector3d(xy*yz - xz*yy, xy*xz - yz*xx, det_z);
      weight = det_z * det_z;

      if (weighted_dir.dot(axis_dir) < 0.0) weight = -weight;

      weighted_dir = weighted_dir + axis_dir * weight;

      Eigen::Vector3d plane_normal = weighted_dir.normalized();
      double inc = std::acos(std::min(1.0, std::abs(plane_normal.z())));
      if (inc != 0) {
        // scaling to size of char so we can send back as much info as possible
        float scaled_inc = static_cast<float>(inc * 255 * 2 / M_PI);
        incs.at<float>(plane_i, plane_j) = std::max(incs.at<float>(plane_i, plane_j), scaled_inc);
      }
    }
  }

	for (size_t mx = 0; mx < xs; ++mx){
		for (size_t my = 0; my < ys; ++my){
			if (!has_data_map.at(mx).at(my)){
        // No pointcloud points fell in this pixel... so don't update
        continue;
			} 
      size_t index = getIndex(mx, my);
      float char_inc = incs.at<float> (mx, my);
      double inc = static_cast<double>(char_inc) * 90 / 255;

      if (inc >= max_safe_inc_){
        costmap_[index] = nav2_costmap_2d::LETHAL_OBSTACLE;
      } else {
        costmap_[index] = static_cast<uint8_t>(nav2_costmap_2d::MAX_NON_OBSTACLE * inc / max_safe_inc_);
      }
		}
	}

  updateFootprint(robot_x, robot_y, robot_yaw, min_x, min_y, max_x, max_y);
}

} // nova_costmap_2d

// Register the macro for this layer
#include "pluginlib/class_list_macros.hpp"
PLUGINLIB_EXPORT_CLASS(nova_costmap_2d::PlaneMapperLayer, nav2_costmap_2d::Layer)