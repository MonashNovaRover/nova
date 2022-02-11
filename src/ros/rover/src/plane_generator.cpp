#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "plane_generator.h"
#include <vector>

#include <bits/stdc++.h>

//#include "rclcpp/rclcpp.hpp"

using namespace std::chrono_literals;

const float CRITICAL_INC = 10.0;
const int MAP_SIZE_X = 2000;
const int MAP_SIZE_Y = 2000;


PlaneGenerator::PlaneGenerator(int num_planes_x, int num_planes_y) 
    : num_planes_x(num_planes_x), num_planes_y(num_planes_y) {
    
    planes = std::vector<std::vector<Plane>> (num_planes_y, std::vector<Plane>(num_planes_x));
}

void PlaneGenerator::add_height_map(HeightMap& height_map) {
    this->height_map = HeightMap(height_map);
    
    plane_x_pixels = height_map.pixel_x / num_planes_x;
    plane_y_pixels = height_map.pixel_y / num_planes_y;
    pixels_per_plane = plane_x_pixels * plane_y_pixels;

    pixel_size_cm = height_map.pixel_size_cm;
}

void PlaneGenerator::fit_planes(HeightMap& h) {
    add_height_map(h);

    fit_planes();
}

void PlaneGenerator::fit_planes() {
    double start = std::clock();

    for (int plane_i = 0; plane_i < num_planes_x; plane_i++) {
        for (int plane_j = 0; plane_j < num_planes_y; plane_j++) {
            Vec3 point_sum;
            //std::vector<Vec3> these_pts;
            Vec3 these_pts[plane_y_pixels][plane_x_pixels];
            for (int i = 0; i < plane_x_pixels; i++) {
                for (int j = 0; j < plane_y_pixels; j++) {
                    int x_index = plane_i * plane_x_pixels + i;
                    int y_index = plane_j * plane_y_pixels + j;

                    float z = get(x_index, y_index);

                    Vec3 p(x_index*pixel_size_cm, y_index*pixel_size_cm, z);
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
            
            // linear regression in y direction
            axis_dir = Vec3(xy*yz - xz*yy, xy*xz - yz*xx, det_z);
            weight = det_z * det_z;

            if (weighted_dir.dot(axis_dir) < 0.0) weight = -weight;

            weighted_dir = weighted_dir + axis_dir * weight;

            Vec3 normal = weighted_dir.normalise();

            Plane plane(0, centroid, normal);
            planes[plane_j][plane_i] = plane;
        }
    }
    double end = std::clock();
    double time = (end - start)  / double(CLOCKS_PER_SEC);
    std::cout << "took " << time << "s" << std::endl;
}

float PlaneGenerator::get(int x, int y) {
    return height_map.get(((float)x) * 2.5, ((float)y) * 2.5);
}



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

HeightMap steep_cliff_terrain(){
    HeightMap h = HeightMap(2000, 2000, 2.5);
    for (float i = 0; i < MAP_SIZE_X; i += 2.5) {
        for (float j = 0; j < MAP_SIZE_Y; j += 2.5) {
            h.add_point(i, j, (int) floor(i / 749) * 1000);
        }
    }

    return h;
}

HeightMap sin_wave_terrain() {
    HeightMap h = HeightMap(2000, 2000, 2.5);
    for (float i = 0; i < MAP_SIZE_X; i += 2.5) {
        for (float j = 0; j < MAP_SIZE_Y; j += 2.5) {
            h.add_point(i, j, sin(i + j) * 1000);
        }
    }

    return h;
}

int main() {
    HeightMap h = sin_wave_terrain();
    PlaneGenerator p = PlaneGenerator(80, 80);
    p.fit_planes(h);
    std::cout << p.planes[0][0].normal.z << std::endl;
}