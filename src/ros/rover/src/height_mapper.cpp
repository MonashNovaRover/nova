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

typedef py::array_t<int16_t> PointCloud;

// pybind thinks every cv::Mat is of unsigned chars, plus they will save us space
static const unsigned char c_neg_inf = 0;
static const unsigned char c_inf = 255;
const unsigned char MAP_BOTTOM = 128;

cv::Mat shift(cv::Mat& original, float x, float y, unsigned char fill_val){
    // shift a cv::Mat in the direction given by x and y.
    float shifter[6] = {1, 0, x, 0, 1, y};
    cv::Mat shift_mat(cv::Size(3, 2), CV_32FC1, shifter);

    cv::Mat shifted;
    cv::warpAffine(original, shifted, shift_mat, original.size());
    
    int start_x = (x >= 0) ? 0 : original.rows - 1;
    int start_y = (y >= 0) ? 0 : original.cols - 1;

    if (x == 0) {
        for (int i = 0; i < original.rows; i++) {
            shifted.at<unsigned char>(i, start_y) = fill_val;
        }
    }
    else if (y == 0) {
        for (int i = 0; i < original.cols; i++) {
            shifted.at<unsigned char>(start_x, i) = fill_val;
        }
    }

    return shifted;
}

void save(cv::Mat& img, cv::Mat& img2, cv::Mat& img3) {
    // use this function to easily display what c++ sees for debugging
    cv::imwrite("../debug/cpp_map.png", img);
    cv::imwrite("../debug/cpp_map2.png", img2);
    cv::imwrite("../debug/cpp_map3.png", img3);
}

py::array_t<unsigned char> getObstacles(PointCloud& points, const int XS, const int YS){
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
    // These two height-maps should sandwich every point in the point cloud
    cv::Mat topHeightMap(cv::Size(YS, XS), CV_8UC1, cv::Scalar(c_neg_inf));
    cv::Mat bottomHeightMap(cv::Size(YS, XS), CV_8UC1, cv::Scalar(c_inf));
    // Finding max and min z for each x-y coordinate
    for (int i = 0; i < pc_info.shape[0] * 3; i+=3) {
            int16_t x = pc[i];
            int16_t y = pc[i + 1];
        if (x >= 0 && y >= 0 && x < XS && y < YS) {
            int16_t z = pc[i + 2] + MAP_BOTTOM; 
            if (z > topHeightMap.at<unsigned char> (x, y)) topHeightMap.at<unsigned char> (x, y) = (unsigned char) z;
            if (z < bottomHeightMap.at<unsigned char> (x, y)) bottomHeightMap.at<unsigned char> (x, y) = (unsigned char) z;
        } else {
            std::cout << "passed invalid index! Fix your shit Max!" << std::endl;
            std::cout << "x = " << x << ", y = " << y << std::endl;
        }
    }
    //blurring the top height map so we can compare the heights of adjacent points
    topHeightMap = cv::max(topHeightMap, shift(topHeightMap, -1, 0, c_neg_inf));
    topHeightMap = cv::max(topHeightMap, shift(topHeightMap, 1, 0, c_neg_inf));
    topHeightMap = cv::max(topHeightMap, shift(topHeightMap, 0, -1, c_neg_inf));
    topHeightMap = cv::max(topHeightMap, shift(topHeightMap, 0, 1, c_neg_inf));
    // blurring the bottom height map so all obstacles are at least 2 pixels wide. Might modify
    // this according to testing
    bottomHeightMap = cv::min(bottomHeightMap, shift(bottomHeightMap, -1, 0, c_inf));
    bottomHeightMap = cv::min(bottomHeightMap, shift(bottomHeightMap, 1, 0, c_inf));
    bottomHeightMap = cv::min(bottomHeightMap, shift(bottomHeightMap, 0, -1, c_inf));
    bottomHeightMap = cv::min(bottomHeightMap, shift(bottomHeightMap, 0, 1, c_inf));
    
    cv::Mat diff;
    cv::subtract(topHeightMap, bottomHeightMap, diff);
    cv::threshold(diff, diff, c_inf, 0, cv::THRESH_TOZERO_INV);

    auto end = std::chrono::high_resolution_clock::now();
    auto time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "obstacle detection took " << time.count() << " microseconds" << std::endl;
    // converting to numpy array to return
    py::array_t<unsigned char> numpy_diff = py::array_t<unsigned char>({ diff.rows, diff.cols }, diff.data);
    save(topHeightMap, bottomHeightMap, diff);
    auto converted = std::chrono::high_resolution_clock::now();
    time = std::chrono::duration_cast<std::chrono::microseconds>(converted - end);
    std::cout << "converting to numpy array took " << time.count() << " microseconds" << std::endl;
    return numpy_diff;
}

PYBIND11_MODULE(height_mapper, module_handle) {
    module_handle.doc() = "Nova Rover height-map obstacle detection algorithm binded to Python3";
    module_handle.def("get_obstacles", &getObstacles); 
}
