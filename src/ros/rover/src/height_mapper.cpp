/*
 
*/


#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <vector>
#include <cmath>
#include <opencv2/opencv.hpp>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

#include <bits/stdc++.h>

//#include "rclcpp/rclcpp.hpp"

using namespace std::chrono_literals;
namespace py = pybind11;

const int XS = 400, YS = 400, ZS = 50;
typedef py::array_t<uint16_t> PointCloud;

// pybind thinks every cv::Mat is of unsigned chars, plus they will save us space
static const unsigned char c_neg_inf = 0;
static const unsigned char c_inf = 255;
const unsigned char MAP_BOTTOM = 1;

cv::Mat shift(cv::Mat& original, float x, float y){
    // shift a cv::Mat in the direction given by x and y.
    float shifter[6] = {1, 0, x, 0, 1, y};
    cv::Mat shift_mat(cv::Size(3, 2), CV_32FC1, shifter);

    cv::Mat shifted;
    cv::warpAffine(original, shifted, shift_mat, original.size());

    return shifted;
}

void save(cv::Mat& img) {
    // use this function to easily display what c++ sees for debugging
    cv::imwrite("../debug/cpp_map.png", img);
}

py::array_t<unsigned char> getObstacles(PointCloud& points){
    py::buffer_info pc_info = points.request();
    uint16_t* pc = static_cast<uint16_t *> (pc_info.ptr);
    int num_pts = pc_info.size / 3;
    
    std::cout << "Received " << num_pts << " points." << std::endl;
    std::cout << "Shape = (" << pc_info.shape[0] << ", " << pc_info.shape[1] << ")" << std::endl;
    std::cout << "ndim = " << pc_info.ndim << std::endl;
    std::cout << "strides = (" << pc_info.strides[0] << ", " << pc_info.strides[1] << ")" << std::endl;
    std::cout << "itemsize = " << pc_info.itemsize << std::endl;
    std::cout << "format = " << pc_info.format << std::endl;

    auto start = std::chrono::high_resolution_clock::now();
    // These two height-maps should sandwhich every point in the point cloud
    cv::Mat topHeightMap(cv::Size(XS, YS), CV_8UC1, cv::Scalar(c_neg_inf));
    cv::Mat bottomHeightMap(cv::Size(XS, YS), CV_8UC1, cv::Scalar(c_inf));
    // Finding max and min z for each x-y coordinate
    for (int i = 0; i < pc_info.shape[0] * 3; i+=3) {
        uint16_t x = pc[i];
        uint16_t y = pc[i + 1];
        uint16_t z = pc[i + 2] + MAP_BOTTOM; 
        if (z > topHeightMap.at<unsigned char> (x, y)) topHeightMap.at<unsigned char> (x, y) = (unsigned char) z;
        if (z < bottomHeightMap.at<unsigned char> (x, y)) bottomHeightMap.at<unsigned char> (x, y) = (unsigned char) z;
    }
                
    //blurring the top height map so we can compare the heights of adjacent points
    topHeightMap = cv::max(topHeightMap, shift(topHeightMap, -1, 0));
    topHeightMap = cv::max(topHeightMap, shift(topHeightMap, 1, 0));
    topHeightMap = cv::max(topHeightMap, shift(topHeightMap, 0, -1));
    topHeightMap = cv::max(topHeightMap, shift(topHeightMap, 0, 1));
    
    cv::Mat diff;
    cv::subtract(topHeightMap, bottomHeightMap, diff);
    cv::threshold(diff, diff, c_inf, 0, cv::THRESH_TOZERO_INV);

    auto end = std::chrono::high_resolution_clock::now();
    auto time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    save(diff);
    std::cout << "obstacle detection took " << time.count() << " microseconds" << std::endl;
    // converting to numpy array to return
    py::array_t<unsigned char> numpy_diff = py::array_t<unsigned char>({ diff.rows, diff.cols }, diff.data);
    
    auto converted = std::chrono::high_resolution_clock::now();
    time = std::chrono::duration_cast<std::chrono::microseconds>(converted - end);
    std::cout << "converting to numpy array took " << time.count() << " microseconds" << std::endl;
    return numpy_diff;
}

PYBIND11_MODULE(height_mapper, module_handle) {
    module_handle.doc() = "Nova Rover height-map obstacle detection algorithm binded to Python3";
    module_handle.def("get_obstacles", &getObstacles); 
}