/*********************************************************************
 *
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2008, 2013, Willow Garage, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 * Author: Max Tory
 *********************************************************************/
#ifndef COSTMAP_HEIGHT_MAPPER_OBSTACLE_LAYER_H_
#define COSTMAP_HEIGHT_MAPPER_OBSTACLE_LAYER_H_

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <cmath>
#include <vector>
#include <limits>
#include <string>
#include "rclcpp/rclcpp.hpp"
#include "nav2_costmap_2d/obstacle_layer.hpp"
#include "nav2_costmap_2d/layered_costmap.hpp"
#include <opencv2/opencv.hpp>
#include <bits/stdc++.h>


namespace nova_costmap_2d
{
/**
 * @class HeightMapperLayer
 * @brief Takes pointcloud data and populates top and bottom heightmap representations of the environment, 
 *        which are used to calculate impassable terrain
*/
class HeightMapperLayer : public nav2_costmap_2d::ObstacleLayer
{
public:
  /**
   * @brief A constructor
  */
  HeightMapperLayer()
  {
    costmap_ = NULL;  // this is the float* member of parent class Costmap2D.
  }

  /**
   * @brief A destructor
  */
  ~HeightMapperLayer();

  /**
   * @brief handles setting up ros subscribers, params, etc. on startup
  */
  virtual void onInitialize();

  /**
   * @brief  Convert from world coordinates to high-resolution HeightMap coordinates
   * @param  wx The x world coordinate
   * @param  wy The y world coordinate
   * @param  mx Will be set to the associated map x coordinate
   * @param  my Will be set to the associated map y coordinate
   * @return True if the conversion was successful (legal bounds) false otherwise
   */
  bool worldToIntermediateMap(double wx, double wy, uint32_t & mx, uint32_t & my) const;

  /**
   * @brief Gets the side length of a single pixel of the intermediate (higher resolution) map
   * @return Resolution in m
  */
  bool getIntermediateResolution(double & res) const;

  /**
   * @brief Update the bounds of the master costmap by this layer's update dimensions
   * @param robot_x X pose of robot
   * @param robot_y Y pose of robot
   * @param robot_yaw Robot orientation
   * @param min_x X min map coord of the window to update
   * @param min_y Y min map coord of the window to update
   * @param max_x X max map coord of the window to update
   * @param max_y Y max map coord of the window to update
   */
  virtual void updateBounds(double robot_x, double robot_y, double robot_yaw, double* min_x, double* min_y,
                            double* max_x, double* max_y);

private:
  /**
   * @brief resolution ratio for convolution to reduce resolution for costmap_
  */
  int resolution_ratio_;

  /**
   * @brief The minimum number of cells required to be occupied for this to be included in height mapping
  */
  float min_plane_density_;

  /**
   * @brief maximum obstacle value that is safe to traverse
  */
  float max_safe_val_;
};

cv::Mat shift(cv::Mat& original, float x, float y, float fill_val){
	// shift a cv::Mat in the direction given by x and y.
	float shifter[6] = {1, 0, x, 0, 1, y};
	cv::Mat shift_mat(cv::Size(3, 2), CV_32FC1, shifter);

	cv::Mat shifted;
	cv::warpAffine(original, shifted, shift_mat, original.size());
	
	int start_x = (x >= 0) ? 0 : shifted.cols - 1;
	int start_y = (y >= 0) ? 0 : shifted.rows - 1;

	if (x == 0) {
			for (int i = 0; i < original.cols; i++) {
					shifted.at<float>(start_y, i) = fill_val;
			}
	}
	else if (y == 0) {
			for (int i = 0; i < original.rows; i++) {
					shifted.at<float>(i, start_x) = fill_val;
			}
	}

	return shifted;
}

}  // namespace nova_costmap_2d

#endif  // COSTMAP_HEIGHT_MAPPER_OBSTACLE_LAYER_H_