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

#include <bits/stdc++.h>

//#include "rclcpp/rclcpp.hpp"

using namespace std::chrono_literals;

const int XS = 200, YS = 200, ZS = 50;
typedef std::array<std::array<std::array<float, ZS>, YS>, XS> Map3D;
typedef cv::Mat Map2D;

static const float c_neg_inf = -9999;
static const float c_inf = 9999;

cv::Mat shift(cv::Mat& original, float x, float y, float replace_val){
    // shift a cv::Mat in the direction given by x and y. The region shifted into
    // the frame is filled with replace_val
    float shifter[6] = {1, 0, x, 0, 1, y};
    cv::Mat shift_mat(cv::Size(3, 2), CV_32FC1, shifter);

    cv::Mat shifted;
    cv::warpAffine(original, shifted, shift_mat, original.size());

    for (int i = 0; i != x; i += x/std::abs(x)) {
        for (int j = 0; j != y; j += y/std::abs(y)) {
            int start_x = (x >= 0) ? 0 : original.size().width - 1;
            int start_y = (y >= 0) ? 0 : original.size().height- 1;
            original.at<float>(start_y + j, start_x + j) = replace_val;   
        }
    }

    return shifted;
}

void print(cv::Mat& img) {
    cv::namedWindow("Display Image", cv::WINDOW_AUTOSIZE );
    cv::resizeWindow("Display Image", 600, 600);
    cv::imshow("Display Image", img);

    cv::waitKey(0);
}

Map2D getObstacles(Map3D& map3D){
    auto start = std::chrono::high_resolution_clock::now();
    // These two height-maps should sandwhich every point in the point cloud
    cv::Mat topHeightMap(cv::Size(XS, YS), CV_32FC1, cv::Scalar(c_neg_inf));
    cv::Mat bottomHeightMap(cv::Size(XS, YS), CV_32FC1, cv::Scalar(c_inf));
    // Finding max and min z for each x-y coordinate
    for (int i = 0; i < XS; i++) {
        for (int j = 0; j < YS; j++) {
            for (int k = 0; k < ZS; k++){
                if (map3D[i][j][k]) {
                    bottomHeightMap.at<float>(i, j) = k;
                    break;
                }
               
            }
            for (int k = ZS - 1; k > -1; k--) {
                if (map3D[i][j][k]) {
                    topHeightMap.at<float>(i, j) = k;
                    break;
                }
            }
        }
    }
                
    //blurring the two height maps so we can compare the heights of adjacent points
    bottomHeightMap = cv::min(bottomHeightMap, shift(bottomHeightMap, -1, 0, c_inf));
    bottomHeightMap = cv::min(bottomHeightMap, shift(bottomHeightMap, 1, 0, c_inf));
    bottomHeightMap = cv::min(bottomHeightMap, shift(bottomHeightMap, 0, -1, c_inf));
    bottomHeightMap = cv::min(bottomHeightMap, shift(bottomHeightMap, 0, 1, c_inf));
    topHeightMap = cv::min(topHeightMap, shift(topHeightMap, -1, 0, c_neg_inf));
    topHeightMap = cv::min(topHeightMap, shift(topHeightMap, 1, 0, c_neg_inf));
    topHeightMap = cv::min(topHeightMap, shift(topHeightMap, 0, -1, c_neg_inf));
    topHeightMap = cv::min(topHeightMap, shift(topHeightMap, 0, 1, c_neg_inf));
    
    Map2D diff = topHeightMap - bottomHeightMap;
    print(diff);
    cv::threshold(diff, diff, c_inf, 0, cv::THRESH_TOZERO_INV);

    auto end = std::chrono::high_resolution_clock::now();
    auto time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "obstacle detection took " << time.count() << " microseconds" << std::endl;
    // converting to numpy array to return
    return diff;
}

int main() {
    Map3D map;
    map.fill({});

    for (int i = 0; i < XS; i++) {
        for (int j = 0; j < YS; j++) {
            map[i][j][21] = 1;
            map[i][j][20] = 1;
        }
    }

    Map2D result = getObstacles(map);

    for (int i = 0; i < XS; i++) {
        for (int j = 0; j < YS; j++) {
            //map[i][j][21] = 1;
            if (!result.at<float>(i, j)){
                std::cout << "x = " << i << ", y = " << j << std::endl;
            }
        }
    }

    print(result);
}
