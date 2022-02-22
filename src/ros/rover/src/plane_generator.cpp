#include "plane_generator.h"
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <array>
#include <cmath>
#include <opencv2/opencv.hpp>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <bits/stdc++.h>

#include <bits/stdc++.h>

//#include "rclcpp/rclcpp.hpp"

using namespace std::chrono_literals;
namespace py = pybind11;

typedef py::array_t<int16_t> PointCloud;

Vec3::Vec3()
    : x(0), y(0), z(0){}

Vec3::Vec3(float x, float y, float z) 
    : x(x), y(y), z(z) {}

Vec3::Vec3(const Vec3& other) 
    : x(other.x), y(other.y), z(other.z) {}

float Vec3::dot(const Vec3& other) {
    return x*other.x + y*other.y + z*other.z; 
}

Vec3 Vec3::cross(const Vec3& other) {
    float x_det = y*other.z - z*other.y;
    float y_det = x*other.z - z*other.x;
    float z_det = x*other.y - y*other.x;

    return Vec3(x_det, -y_det, z_det);
}

Vec3 Vec3::operator+(const Vec3& other) {
    return Vec3 (x+other.x, y+other.y, z+other.z);
}

Vec3 Vec3::operator-(const Vec3& other) {
    return Vec3 (x-other.x, y-other.y, z-other.z);
}

Vec3 Vec3::operator*(float other) {
    return Vec3 (x*other, y*other, z*other);
}

Vec3 Vec3::operator/(float other) {
    return Vec3 (x/other, y/other, z/other);
}

float Vec3::magnitude(){
    return this->dot(*this);
}

Vec3 Vec3::normalise(){
    return Vec3(*this) / sqrt(this->magnitude());
}

// pybind thinks every cv::Mat is of unsigned chars, plus they will save us space
static const unsigned char c_neg_inf = 0;
static const unsigned char c_inf = 255;
static const unsigned char MAP_ZERO = 128;

void save(cv::Mat& img, cv::Mat& img2) {
    // use this function to easily display what c++ sees for debugging
    cv::imwrite("../debug/cpp_heightmap.png", img);
    cv::imwrite("../debug/cpp_heightmap2.png", img2);
}

cv::Mat fit_planes(cv::Mat& heightMap, cv::Mat& incs) {
    const std::size_t XS = heightMap.rows;
    const std::size_t YS = heightMap.cols;

    const std::size_t plane_xs = incs.rows;
    const std::size_t plane_ys = incs.cols;

    const std::size_t plane_x_pixels = 2 * XS/plane_xs;
    const std::size_t plane_y_pixels = 2 * YS/plane_ys;

    std::cout << "XS = " << XS << ", YS = " << YS << std::endl;

    int pixels_per_plane = plane_x_pixels * plane_y_pixels;

    for (std::size_t plane_i = 0; plane_i < plane_xs - 1; plane_i++) {
        for (std::size_t plane_j = 0; plane_j < plane_ys - 1; plane_j++) {
            Vec3 point_sum;
            std::vector<Vec3> these_pts;
            for (std::size_t i = 0; i < plane_x_pixels; i++) {
                for (std::size_t j = 0; j < plane_y_pixels; j++) {
                    int x_index = plane_i * plane_x_pixels/2 + i;
                    int y_index = plane_j * plane_y_pixels/2 + j;

                    int z = heightMap.at<uint8_t>(x_index, y_index);
                    if (z == 0) continue;
                    Vec3 p(x_index, y_index, z);
                    these_pts.push_back(p);
                    point_sum = point_sum + p;
                }
            }

            if (these_pts.size() == 0) continue;

            Vec3 centroid = point_sum / pixels_per_plane;

            double xx=0.0, yy=0.0, zz=0.0, xy=0.0, xz=0.0, yz=0.0;

            for (Vec3 p : these_pts) {
                Vec3 r = p - centroid;
                xx += r.x*r.x;
                xy += r.x*r.y;
                xz += r.x*r.z;
                yy += r.y*r.y;
                yz += r.y*r.z;
                zz += r.z*r.z; 
            }

            xx /= pixels_per_plane;
            xy /= pixels_per_plane;
            xz /= pixels_per_plane;
            yy /= pixels_per_plane;
            yz /= pixels_per_plane;
            zz /= pixels_per_plane;

            Vec3 weighted_dir, axis_dir;
            double weight;

            double det_x = yy*zz - yz*yz;
            double det_y = xx*zz - xz*xz;
            double det_z = xx*yy - xy*xy;

            // linear regression in x direction
            axis_dir = Vec3(det_x, xz*yz - xy*zz, xy*yz - xz*yy);
            weight = det_x * det_x;

            weighted_dir = weighted_dir + axis_dir * weight;

            // linear regression in y direction
            axis_dir = Vec3(xz*yz - xy*zz, det_y, xy*xz - yz*xx);
            weight = det_y * det_y;

            if (weighted_dir.dot(axis_dir) < 0.0) weight = -weight;

            weighted_dir = weighted_dir + axis_dir * weight;
            
            // linear regression in z direction
            axis_dir = Vec3(xy*yz - xz*yy, xy*xz - yz*xx, det_z);
            weight = det_z * det_z;

            if (weighted_dir.dot(axis_dir) < 0.0) weight = -weight;

            weighted_dir = weighted_dir + axis_dir * weight;

            Vec3 normal = weighted_dir.normalise();
            float inc = std::acos(std::abs(normal.z));

            // scaling to size of char so we can send back as much info as possible
            uint8_t scaled_inc = inc * 255 * 2/M_PI;

            for (std::size_t x = plane_i; x < plane_i + 2; x++) {
                for (std::size_t y = plane_j; y < plane_j + 2; y++) {
                    incs.at<uint8_t>(x, y) = std::max(scaled_inc, incs.at<uint8_t>(x, y));
                }
            }
        }
    }

    return incs;
}

py::array_t<uint8_t> getObstacles(PointCloud& points, std::size_t XS, std::size_t YS){
    auto start = std::chrono::high_resolution_clock::now();
    py::buffer_info pc_info = points.request();
    int16_t* pc = static_cast<int16_t *> (pc_info.ptr);

    // These two height-maps should sandwich every point in the point cloud
    cv::Mat heightMap(cv::Size(YS, XS), CV_8UC1, cv::Scalar(c_neg_inf));
    // Finding max and min z for each x-y coordinate
    for (int i = 0; i < pc_info.shape[0] * 3; i+=3) {
            int x = pc[i];
            int y = pc[i + 1];
        if (x >= 0 && y >= 0 && x < (int)XS && y < (int)YS) {
            uint8_t z = pc[i + 2] + MAP_ZERO; 
            if (z > heightMap.at<uint8_t>(x, y)) heightMap.at<uint8_t>(x, y) = (unsigned char) z;
        } else {
            std::cout << "passed invalid index! Fix your shit Max!" << std::endl;
            std::cout << "x = " << x << ", y = " << y << std::endl;
        }
    }

    // size of the map of planes with scaled down resolution
    std::size_t xs = XS/4, ys = YS/4;
    cv::Mat obstacleMap(cv::Size(ys, xs), CV_8UC1, cv::Scalar(c_neg_inf));
    fit_planes(heightMap, obstacleMap);
    
    // converting to numpy array
    py::array_t<unsigned char> numpy_obs = py::array_t<unsigned char>({ obstacleMap.rows, obstacleMap.cols }, obstacleMap.data);

    auto end = std::chrono::high_resolution_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    save(heightMap, obstacleMap);

    std::cout << "Plane fitting took " << diff.count() << " microseconds." << std::endl;

    return numpy_obs;
}

PYBIND11_MODULE(plane_fitter, module_handle) {
    module_handle.doc() = "Nova Rover plane-fitting obstacle detection algorithm binded to Python3";
    module_handle.def("get_obstacles", &getObstacles); 
}
