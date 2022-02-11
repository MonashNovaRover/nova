#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <cmath>
//#include "height_map.h"
//#include "rclcpp/rclcpp.hpp"

//using namespace std::chrono_literals;

class HeightMap {
public:
    int x_cm, y_cm;
    int pixel_x, pixel_y;

    std::vector<std::vector<float>> map;
    float pixel_size_cm;

    HeightMap(const HeightMap& other)
        : x_cm(other.x_cm), y_cm(other.y_cm), pixel_x(other.pixel_x),
        pixel_y(other.pixel_y), map(other.map), pixel_size_cm(other.pixel_size_cm)
        {}

    HeightMap(int x, int y, float pixel_size_cm)
        : x_cm(x), y_cm(y), pixel_size_cm(pixel_size_cm) {
        
        pixel_x = (int) (x_cm / pixel_size_cm);
        pixel_y = (int) (y_cm / pixel_size_cm);

        map = std::vector<std::vector<float>> (pixel_y, std::vector<float> (pixel_x));
    }

    HeightMap(float pixel_size_cm) 
        : pixel_size_cm(pixel_size_cm) {}

    HeightMap()
        : pixel_size_cm(0) {}

    void add_map(std::vector<std::vector<float>> new_map) {
        map = new_map;

        pixel_x = new_map[0].size();
        pixel_y = new_map.size();

        x_cm = pixel_x * pixel_size_cm;
        y_cm = pixel_y * pixel_size_cm;
    }

    void add_point(float x, float y, float z) {
        int x_index = (int) (x / pixel_size_cm);
        int y_index = (int) (y / pixel_size_cm);

        map[y_index][x_index] = z;
    }

    float get(float x, float y) {
        int x_index = (int) std::floor(x / pixel_size_cm);
        int y_index = (int) std::floor(y / pixel_size_cm);

        return map[y_index][x_index];
    }
}; 
