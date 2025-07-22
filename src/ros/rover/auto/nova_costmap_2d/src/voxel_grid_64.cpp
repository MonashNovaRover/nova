#include <nova_costmap_2d/voxel_grid_64.hpp>

namespace nav2_voxel_grid_64
{
VoxelGrid::VoxelGrid(unsigned int size_x, unsigned int size_y, unsigned int size_z)
: logger(rclcpp::get_logger("voxel_grid"))
{
  size_x_ = size_x;
  size_y_ = size_y;
  size_z_ = size_z;

  if (size_z_ > 32) {
    RCLCPP_INFO(
      logger, "Error, this implementation can only support up to 32 z values (%d)",
      size_z_);
    size_z_ = 32;
  }

  data_ = new uint64_t[size_x_ * size_y_];
  uint64_t unknown_col = ~((uint64_t)0) >> 32;
  uint64_t * col = data_;
  for (unsigned int i = 0; i < size_x_ * size_y_; ++i) {
    *col = unknown_col;
    ++col;
  }
}

void VoxelGrid::resize(unsigned int size_x, unsigned int size_y, unsigned int size_z)
{
  // if we're not actually changing the size, we can just reset things
  if (size_x == size_x_ && size_y == size_y_ && size_z == size_z_) {
    reset();
    return;
  }

  delete[] data_;
  size_x_ = size_x;
  size_y_ = size_y;
  size_z_ = size_z;

  if (size_z_ > 32) {
    RCLCPP_INFO(
      logger, "Error, this implementation can only support up to 32 z values (%d)",
      size_z);
    size_z_ = 32;
  }

  data_ = new uint64_t[size_x_ * size_y_];
  uint64_t unknown_col = ~((uint64_t)0) >> 32;
  uint64_t * col = data_;
  for (unsigned int i = 0; i < size_x_ * size_y_; ++i) {
    *col = unknown_col;
    ++col;
  }
}

VoxelGrid::~VoxelGrid()
{
  delete[] data_;
}

void VoxelGrid::reset()
{
  uint64_t unknown_col = ~((uint64_t)0) >> 32;
  uint64_t * col = data_;
  for (unsigned int i = 0; i < size_x_ * size_y_; ++i) {
    *col = unknown_col;
    ++col;
  }
}

void VoxelGrid::markVoxelLine(
  double x0, double y0, double z0, double x1, double y1, double z1,
  unsigned int max_length)
{
  if (x0 >= size_x_ || y0 >= size_y_ || z0 >= size_z_ || x1 >= size_x_ || y1 >= size_y_ ||
    z1 >= size_z_)
  {
    RCLCPP_DEBUG(
      logger,
      "Error, line endpoint out of bounds. "
      "(%.2f, %.2f, %.2f) to (%.2f, %.2f, %.2f),  size: (%d, %d, %d)",
      x0, y0, z0, x1, y1, z1, size_x_, size_y_, size_z_);
    return;
  }

  MarkVoxel mv(data_);
  raytraceLine(mv, x0, y0, z0, x1, y1, z1, max_length);
}

void VoxelGrid::clearVoxelLine(
  double x0, double y0, double z0, double x1, double y1, double z1,
  unsigned int max_length, unsigned int min_length)
{
  if (x0 >= size_x_ || y0 >= size_y_ || z0 >= size_z_ || x1 >= size_x_ || y1 >= size_y_ ||
    z1 >= size_z_)
  {
    RCLCPP_DEBUG(
      logger,
      "Error, line endpoint out of bounds. "
      "(%.2f, %.2f, %.2f) to (%.2f, %.2f, %.2f),  size: (%d, %d, %d)",
      x0, y0, z0, x1, y1, z1, size_x_, size_y_, size_z_);
    return;
  }

  ClearVoxel cv(data_);
  raytraceLine(cv, x0, y0, z0, x1, y1, z1, max_length, min_length);
}

void VoxelGrid::clearVoxelLineInMap(
  double x0, double y0, double z0, double x1, double y1, double z1, unsigned char * map_2d,
  unsigned int unknown_threshold, unsigned int mark_threshold, unsigned char free_cost,
  unsigned char unknown_cost, unsigned int max_length, unsigned int min_length)
{
  costmap = map_2d;
  if (map_2d == NULL) {
    clearVoxelLine(x0, y0, z0, x1, y1, z1, max_length, min_length);
    return;
  }

  if (x0 >= size_x_ || y0 >= size_y_ || z0 >= size_z_ || x1 >= size_x_ || y1 >= size_y_ ||
    z1 >= size_z_)
  {
    RCLCPP_DEBUG(
      logger,
      "Error, line endpoint out of bounds. "
      "(%.2f, %.2f, %.2f) to (%.2f, %.2f, %.2f),  size: (%d, %d, %d)",
      x0, y0, z0, x1, y1, z1, size_x_, size_y_, size_z_);
    return;
  }

  ClearVoxelInMap cvm(data_, costmap, unknown_threshold, mark_threshold, free_cost, unknown_cost);
  raytraceLine(cvm, x0, y0, z0, x1, y1, z1, max_length, min_length);
}

VoxelStatus VoxelGrid::getVoxel(unsigned int x, unsigned int y, unsigned int z)
{
  if (x >= size_x_ || y >= size_y_ || z >= size_z_) {
    RCLCPP_DEBUG(logger, "Error, voxel out of bounds. (%d, %d, %d)\n", x, y, z);
    return UNKNOWN;
  }
  uint64_t full_mask = ((uint64_t)1 << z << 32) | (1 << z);
  uint64_t result = data_[y * size_x_ + x] & full_mask;
  unsigned int bits = numBits(result);

  // known marked: 11 = 2 bits, unknown: 01 = 1 bit, known free: 00 = 0 bits
  if (bits < 2) {
    if (bits < 1) {
      return FREE;
    }

    return UNKNOWN;
  }

  return MARKED;
}

VoxelStatus VoxelGrid::getVoxelColumn(
  unsigned int x, unsigned int y,
  unsigned int unknown_threshold, unsigned int marked_threshold)
{
  if (x >= size_x_ || y >= size_y_) {
    RCLCPP_DEBUG(logger, "Error, voxel out of bounds. (%d, %d)\n", x, y);
    return UNKNOWN;
  }

  uint64_t * col = &data_[y * size_x_ + x];

  unsigned int unknown_bits = uint32_t(*col >> 32) ^ uint32_t(*col);
  unsigned int marked_bits = *col >> 32;

  // check if the number of marked bits qualifies the col as marked
  if (!bitsBelowThreshold(marked_bits, marked_threshold)) {
    return MARKED;
  }

  // check if the number of unkown bits qualifies the col as unknown
  if (!bitsBelowThreshold(unknown_bits, unknown_threshold)) {
    return UNKNOWN;
  }

  return FREE;
}

unsigned int VoxelGrid::sizeX()
{
  return size_x_;
}

unsigned int VoxelGrid::sizeY()
{
  return size_y_;
}

unsigned int VoxelGrid::sizeZ()
{
  return size_z_;
}

void VoxelGrid::printVoxelGrid()
{
  for (unsigned int z = 0; z < size_z_; z++) {
    printf("Layer z = %u:\n", z);
    for (unsigned int y = 0; y < size_y_; y++) {
      for (unsigned int x = 0; x < size_x_; x++) {
        printf((getVoxel(x, y, z)) == nav2_voxel_grid::MARKED ? "#" : " ");
      }
      printf("|\n");
    }
  }
}

void VoxelGrid::printColumnGrid()
{
  printf("Column view:\n");
  for (unsigned int y = 0; y < size_y_; y++) {
    for (unsigned int x = 0; x < size_x_; x++) {
      printf((getVoxelColumn(x, y, 32, 0) == nav2_voxel_grid::MARKED) ? "#" : " ");
    }
    printf("|\n");
  }
}
}  // namespace nav2_voxel_grid_64