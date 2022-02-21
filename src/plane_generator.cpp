#include "plane_generator.h"
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <array>
#include <cmath>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <bits/stdc++.h>

#include <bits/stdc++.h>

//#include "rclcpp/rclcpp.hpp"

using namespace std::chrono_literals;
namespace py = pybind11;

typedef py::array_t<int16_t> PointCloud;

Plane::Plane(int num_points, Vec3 centroid, Vec3 normal) 
    : num_points(num_points), centroid(centroid), normal(normal) {}

Plane::Plane(){}

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
const unsigned char MAP_BOTTOM = 128;
const Vec3 UP(0, 0, 1);

template <std::size_t XS, std::size_t YS>
std::array<std::array<float, YS>, XS> fit_planes(std::array<std::array<uint8_t, 4*YS>, 4*XS> heightMap,
                                                std::array<std::array<float, YS>, XS>& incs) {

    double start = std::clock();
    std::size_t plane_xs = (std::size_t) std::floor(XS/4);
    std::size_t plane_ys = (std::size_t) std::floor(YS/4);
    
    int plane_x_pixels = 8, plane_y_pixels = 8;
    int pixels_per_plane = plane_x_pixels * plane_y_pixels;

    for (int plane_i = 0; plane_i < plane_xs - 1; plane_i++) {
        for (int plane_j = 0; plane_j < plane_ys - 1; plane_j++) {
            Vec3 point_sum;
            //std::vector<Vec3> these_pts;

            std::array<std::array<Vec3, 8>, 8> these_pts;
            for (int i = 0; i < plane_x_pixels; i++) {
                for (int j = 0; j < plane_y_pixels; j++) {
                    int x_index = plane_i * plane_x_pixels + i;
                    int y_index = plane_j * plane_y_pixels + j;

                    float z = heightMap[x_index][y_index];

                    Vec3 p(x_index, y_index, z);
                    these_pts[j][i] = p;
                    point_sum = point_sum + p;
                }
            }

            Vec3 centroid = point_sum / pixels_per_plane;

            double xx=0.0, yy=0.0, zz=0.0, xy=0.0, xz=0.0, yz=0.0;

            for (int i = 0; i < plane_x_pixels; i++) {
                for (int j = 0; j < plane_y_pixels; j++) {
                    Vec3 p = these_pts[j][i];
                    Vec3 r = p - centroid;
                    xx += r.x*r.x;
                    xy += r.x*r.y;
                    xz += r.x*r.z;
                    yy += r.y*r.y;
                    yz += r.y*r.z;
                    zz += r.z*r.z; 
                }
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
            float inc = std::acos(std::abs(normal.dot(UP)));
            for (int x = plane_i; x < plane_i + 2; x = plane_i + 1) {
                for (int y = plane_j; y < plane_j + 2; y = plane_j + 1) {
                    incs[x][y] = std::max(inc, incs[x][y]);
                }
            }
        }
    }
    double end = std::clock();
    double time = (end - start)  / double(CLOCKS_PER_SEC);
    std::cout << "Plane fitting took " << time << "s" << std::endl;

    return incs;
}

template <std::size_t XS, std::size_t YS>
std::array<std::array<float, (std::size_t)std::floor(YS/4)>, (std::size_t)std::floor(XS/4)> 
getObstacles(PointCloud& points, std::array<std::array<float, YS>, XS>& incs){
    py::buffer_info pc_info = points.request();
    uint16_t* pc = static_cast<uint16_t *> (pc_info.ptr);

    auto start = std::chrono::high_resolution_clock::now();
    // These two height-maps should sandwich every point in the point cloud
    std::array<std::array<uint8_t, YS>, XS> heightMap = {};
    // Finding max and min z for each x-y coordinate
    for (int i = 0; i < pc_info.shape[0] * 3; i+=3) {
            int x = pc[i];
            int y = pc[i + 1];
        if (x >= 0 && y >= 0 && x < XS && y < YS) {
            int8_t z = pc[i + 2] + MAP_BOTTOM; 
            if (z > heightMap[x][y]) heightMap[x][y] = (unsigned char) z;
        } else {
            std::cout << "passed invalid index! Fix your shit Max!" << std::endl;
            std::cout << "x = " << x << ", y = " << y << std::endl;
        }
    }

    return fit_planes(heightMap, incs);
}

PYBIND11_MODULE(plane_fitter, module_handle) {
    module_handle.doc() = "Nova Rover plane-fitting obstacle detection algorithm binded to Python3";
    module_handle.def<200, 200>("get_obstacles", &getObstacles); 
}
