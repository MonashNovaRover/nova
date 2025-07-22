/*********************************************************************
 *
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2008, Willow Garage, Inc.
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
 *   * Neither the name of the Willow Garage nor the names of its
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
 * Author: Eitan Marder-Eppstein
 *
 * Extends nav2_voxel_grid::VoxelGrid to offer a version stored as uint64_t, allowing for 
 * 32 layers vertically.
 * Modified: Max Tory
 *********************************************************************/
#ifndef NAV2_VOXEL_GRID_64__VOXEL_GRID_64_HPP_
#define NAV2_VOXEL_GRID_64__VOXEL_GRID_64_HPP_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <limits.h>
#include <algorithm>
#include "rclcpp/rclcpp.hpp"

/**
 * @class VoxelGrid
 * @brief A 3D grid structure that stores points as an integer array.
 *        X and Y index the array and Z selects which bit of the integer
 *        is used giving a limit of 32 vertical cells.
 */
namespace nav2_voxel_grid_64
{

enum VoxelStatus
{
  FREE = 0,
  UNKNOWN = 1,
  MARKED = 2,
};

class VoxelGrid64
{
public:
  /**
   * @brief  Constructor for a voxel grid
   * @param size_x The x size of the grid
   * @param size_y The y size of the grid
   * @param size_z The z size of the grid, only sizes <= 32 are supported
   */
  VoxelGrid64(uint32_t size_x, uint32_t size_y, uint32_t size_z);

  ~VoxelGrid64();

  /**
   * @brief  Resizes a voxel grid to the desired size
   * @param size_x The x size of the grid
   * @param size_y The y size of the grid
   * @param size_z The z size of the grid, only sizes <= 32 are supported
   */
  void resize(uint32_t size_x, uint32_t size_y, uint32_t size_z);

  void reset();
  uint64_t * getData() {return data_;}

  inline void markVoxel(uint32_t x, uint32_t y, uint32_t z)
  {
    if (x >= size_x_ || y >= size_y_ || z >= size_z_) {
      RCLCPP_DEBUG(logger, "Error, voxel out of bounds.\n");
      return;
    }
    uint64_t full_mask = ((uint64_t)1 << z << 32) | (1 << z);
    data_[y * size_x_ + x] |= full_mask;  // clear unknown and mark cell
  }

  inline bool markVoxelInMap(
    uint32_t x, uint32_t y, uint32_t z,
    uint32_t marked_threshold)
  {
    if (x >= size_x_ || y >= size_y_ || z >= size_z_) {
      RCLCPP_DEBUG(logger, "Error, voxel out of bounds.\n");
      return false;
    }

    int index = y * size_x_ + x;
    uint64_t * col = &data_[index];
    uint64_t full_mask = ((uint64_t)1 << z << 32) | (1 << z);
    *col |= full_mask;  // clear unknown and mark cell

    uint32_t marked_bits = *col >> 32;

    // make sure the number of bits in each is below our thresholds
    return !bitsBelowThreshold(marked_bits, marked_threshold);
  }

  inline void clearVoxel(uint32_t x, uint32_t y, uint32_t z)
  {
    if (x >= size_x_ || y >= size_y_ || z >= size_z_) {
      RCLCPP_DEBUG(logger, "Error, voxel out of bounds.\n");
      return;
    }
    uint64_t full_mask = ((uint64_t)1 << z << 32) | (1 << z);
    data_[y * size_x_ + x] &= ~(full_mask);  // clear unknown and clear cell
  }

  inline void clearVoxelColumn(uint32_t index)
  {
    assert(index < size_x_ * size_y_);
    data_[index] = 0;
  }

  inline void clearVoxelInMap(uint32_t x, uint32_t y, uint32_t z)
  {
    if (x >= size_x_ || y >= size_y_ || z >= size_z_) {
      RCLCPP_DEBUG(logger, "Error, voxel out of bounds.\n");
      return;
    }
    int index = y * size_x_ + x;
    uint64_t * col = &data_[index];
    uint64_t full_mask = ((uint64_t)1 << z << 32) | (1 << z);
    *col &= ~(full_mask);  // clear unknown and clear cell

    uint32_t unknown_bits = uint32_t(*col >> 32) ^ uint32_t(*col);
    uint32_t marked_bits = *col >> 32;

    // make sure the number of bits in each is below our thresholds
    if (bitsBelowThreshold(unknown_bits, 1) && bitsBelowThreshold(marked_bits, 1)) {
      costmap[index] = 0;
    }
  }

  inline bool bitsBelowThreshold(uint32_t n, uint32_t bit_threshold)
  {
    uint32_t bit_count;
    for (bit_count = 0; n; ) {
      ++bit_count;
      if (bit_count > bit_threshold) {
        return false;
      }
      n &= n - 1;  // clear the least significant bit set
    }
    return true;
  }

  static inline uint32_t numBits(uint32_t n)
  {
    uint32_t bit_count;
    for (bit_count = 0; n; ++bit_count) {
      n &= n - 1;  // clear the least significant bit set
    }
    return bit_count;
  }

  static VoxelStatus getVoxel(
    uint32_t x, uint32_t y, uint32_t z,
    uint32_t size_x, uint32_t size_y, uint32_t size_z, const uint64_t * data)
  {
    if (x >= size_x || y >= size_y || z >= size_z) {
      return UNKNOWN;
    }
    uint64_t full_mask = ((uint64_t)1 << z << 32) | (1 << z);
    uint64_t result = data[y * size_x + x] & full_mask;
    uint32_t bits = numBits(result);

    // known marked: 11 = 2 bits, unknown: 01 = 1 bit, known free: 00 = 0 bits
    if (bits < 2) {
      if (bits < 1) {
        return FREE;
      }
      return UNKNOWN;
    }
    return MARKED;
  }

  void markVoxelLine(
    double x0, double y0, double z0, double x1, double y1, double z1,
    uint32_t max_length = UINT_MAX);
  void clearVoxelLine(
    double x0, double y0, double z0, double x1, double y1, double z1,
    uint32_t max_length = UINT_MAX, uint32_t min_length = 0);
  void clearVoxelLineInMap(
    double x0, double y0, double z0, double x1, double y1, double z1, uint8_t * map_2d,
    uint32_t unknown_threshold, uint32_t mark_threshold,
    uint8_t free_cost = 0, uint8_t unknown_cost = 255,
    uint32_t max_length = UINT_MAX, uint32_t min_length = 0);

  VoxelStatus getVoxel(uint32_t x, uint32_t y, uint32_t z);

  // Are there any obstacles at that (x, y) location in the grid?
  VoxelStatus getVoxelColumn(
    uint32_t x, uint32_t y,
    uint32_t unknown_threshold = 0, uint32_t marked_threshold = 0);

  void printVoxelGrid();
  void printColumnGrid();
  uint32_t sizeX();
  uint32_t sizeY();
  uint32_t sizeZ();

  template<class ActionType>
  inline void raytraceLine(
    ActionType at, double x0, double y0, double z0,
    double x1, double y1, double z1, uint32_t max_length = UINT_MAX,
    uint32_t min_length = 0)
  {
    // we need to chose how much to scale our dominant dimension, based on the
    // maximum length of the line
    double dist = sqrt((x0 - x1) * (x0 - x1) + (y0 - y1) * (y0 - y1) + (z0 - z1) * (z0 - z1));
    if ((uint32_t)(dist) < min_length) {
      return;
    }
    double scale, min_x0, min_y0, min_z0;
    if (dist > 0.0) {
      scale = std::min(1.0, max_length / dist);

      // Updating starting point to the point at distance min_length from the initial point
      min_x0 = x0 + (x1 - x0) / dist * min_length;
      min_y0 = y0 + (y1 - y0) / dist * min_length;
      min_z0 = z0 + (z1 - z0) / dist * min_length;
    } else {
      // dist can be 0 if [x0, y0, z0]==[x1, y1, z1].
      // In this case only this voxel should be processed.
      scale = 1.0;
      min_x0 = x0;
      min_y0 = y0;
      min_z0 = z0;
    }

    int dx = int(x1) - int(min_x0);  // NOLINT
    int dy = int(y1) - int(min_y0);  // NOLINT
    int dz = int(z1) - int(min_z0);  // NOLINT

    uint32_t abs_dx = abs(dx);
    uint32_t abs_dy = abs(dy);
    uint32_t abs_dz = abs(dz);

    int offset_dx = sign(dx);
    int offset_dy = sign(dy) * size_x_;
    int offset_dz = sign(dz);

    uint32_t z_mask = ((1 << 32) | 1) << (uint32_t)min_z0;
    uint32_t offset = (uint32_t)min_y0 * size_x_ + (uint32_t)min_x0;

    GridOffset grid_off(offset);
    ZOffset z_off(z_mask);

    // is x dominant
    if (abs_dx >= max(abs_dy, abs_dz)) {
      int error_y = abs_dx / 2;
      int error_z = abs_dx / 2;

      bresenham3D(
        at, grid_off, grid_off, z_off, abs_dx, abs_dy, abs_dz, error_y, error_z,
        offset_dx, offset_dy, offset_dz, offset, z_mask, (uint32_t)(scale * abs_dx));
      return;
    }

    // y is dominant
    if (abs_dy >= abs_dz) {
      int error_x = abs_dy / 2;
      int error_z = abs_dy / 2;

      bresenham3D(
        at, grid_off, grid_off, z_off, abs_dy, abs_dx, abs_dz, error_x, error_z,
        offset_dy, offset_dx, offset_dz, offset, z_mask, (uint32_t)(scale * abs_dy));
      return;
    }

    // otherwise, z is dominant
    int error_x = abs_dz / 2;
    int error_y = abs_dz / 2;

    bresenham3D(
      at, z_off, grid_off, grid_off, abs_dz, abs_dx, abs_dy, error_x, error_y, offset_dz,
      offset_dx, offset_dy, offset, z_mask, (uint32_t)(scale * abs_dz));
  }

private:
  // the real work is done here... 3D bresenham implementation
  template<class ActionType, class OffA, class OffB, class OffC>
  inline void bresenham3D(
    ActionType at, OffA off_a, OffB off_b, OffC off_c,
    uint32_t abs_da, uint32_t abs_db, uint32_t abs_dc,
    int error_b, int error_c, int offset_a, int offset_b, int offset_c, uint32_t & offset,
    uint32_t & z_mask, uint32_t max_length = UINT_MAX)
  {
    uint32_t end = std::min(max_length, abs_da);
    for (uint32_t i = 0; i < end; ++i) {
      at(offset, z_mask);
      off_a(offset_a);
      error_b += abs_db;
      error_c += abs_dc;
      if ((uint32_t)error_b >= abs_da) {
        off_b(offset_b);
        error_b -= abs_da;
      }
      if ((uint32_t)error_c >= abs_da) {
        off_c(offset_c);
        error_c -= abs_da;
      }
    }
    at(offset, z_mask);
  }

  inline int sign(int i)
  {
    return i > 0 ? 1 : -1;
  }

  inline uint32_t max(uint32_t x, uint32_t y)
  {
    return x > y ? x : y;
  }

  uint32_t size_x_, size_y_, size_z_;
  uint64_t * data_;
  uint8_t * costmap;
  rclcpp::Logger logger;

  // Aren't functors so much fun... used to recreate the Bresenham macro Eric
  // wrote in the original version, but in "proper" c++
  class MarkVoxel
  {
public:
    explicit MarkVoxel(uint64_t * data)
    : data_(data) {}
    inline void operator()(uint32_t offset, uint32_t z_mask)
    {
      data_[offset] |= z_mask;  // clear unknown and mark cell
    }

private:
    uint64_t * data_;
  };

  class ClearVoxel
  {
public:
    explicit ClearVoxel(uint64_t * data)
    : data_(data) {}
    inline void operator()(uint32_t offset, uint32_t z_mask)
    {
      data_[offset] &= ~(z_mask);  // clear unknown and clear cell
    }

private:
    uint64_t * data_;
  };

  class ClearVoxelInMap
  {
public:
    ClearVoxelInMap(
      uint64_t * data, uint8_t * costmap,
      uint32_t unknown_clear_threshold, uint32_t marked_clear_threshold,
      uint8_t free_cost = 0, uint8_t unknown_cost = 255)
    : data_(data), costmap_(costmap),
      unknown_clear_threshold_(unknown_clear_threshold), marked_clear_threshold_(
        marked_clear_threshold),
      free_cost_(free_cost), unknown_cost_(unknown_cost)
    {
    }

    inline void operator()(uint32_t offset, uint32_t z_mask)
    {
      uint64_t * col = &data_[offset];
      *col &= ~(z_mask);  // clear unknown and clear cell

      uint32_t unknown_bits = uint32_t(*col >> 32) ^ uint32_t(*col);
      uint32_t marked_bits = *col >> 32;

      // make sure the number of bits in each is below our thresholds
      if (bitsBelowThreshold(marked_bits, marked_clear_threshold_)) {
        if (bitsBelowThreshold(unknown_bits, unknown_clear_threshold_)) {
          costmap_[offset] = free_cost_;
        } else {
          costmap_[offset] = unknown_cost_;
        }
      }
    }

private:
    inline bool bitsBelowThreshold(uint32_t n, uint32_t bit_threshold)
    {
      uint32_t bit_count;
      for (bit_count = 0; n; ) {
        ++bit_count;
        if (bit_count > bit_threshold) {
          return false;
        }
        n &= n - 1;  // clear the least significant bit set
      }
      return true;
    }

    uint64_t * data_;
    uint8_t * costmap_;
    uint32_t unknown_clear_threshold_, marked_clear_threshold_;
    uint8_t free_cost_, unknown_cost_;
  };

  class GridOffset
  {
public:
    explicit GridOffset(uint32_t & offset)
    : offset_(offset) {}
    inline void operator()(int offset_val)
    {
      offset_ += offset_val;
    }

private:
    uint32_t & offset_;
  };

  class ZOffset
  {
public:
    explicit ZOffset(uint32_t & z_mask)
    : z_mask_(z_mask) {}
    inline void operator()(int offset_val)
    {
      offset_val > 0 ? z_mask_ <<= 1 : z_mask_ >>= 1;
    }

private:
    uint32_t & z_mask_;
  };
};

}  // namespace nav2_voxel_grid_64

#endif  // NAV2_VOXEL_GRID_64__VOXEL_GRID_64_HPP_******************************************************