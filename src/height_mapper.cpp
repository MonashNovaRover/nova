#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <vector>
#include <cmath>

#include <bits/stdc++.h>

//#include "rclcpp/rclcpp.hpp"

using namespace std::chrono_literals;
const int XS = 200, YS = 200, ZS = 50;
typedef std::array<std::array<std::array<float, ZS>, YS>, XS> Map3D;
typedef std::array<std::array<float, YS>, XS> Map2D;

class Vec3{
public:
    float x, y, z;

    Vec3()
        : x(0), y(0), z(0){}

    Vec3(float x, float y, float z) 
        : x(x), y(y), z(z) {}

    Vec3(const Vec3& other) 
        : x(other.x), y(other.y), z(other.z) {}

	float dot(const Vec3& other) {
        return x*other.x + y*other.y + z*other.z; 
    }

    Vec3 cross(const Vec3& other) {
        float x_det = y*other.z - z*other.y;
        float y_det = x*other.z - z*other.x;
        float z_det = x*other.y - y*other.x;

        return Vec3(x_det, -y_det, z_det);
    }

    Vec3 operator+(const Vec3& other) {
        return Vec3 (x+other.x, y+other.y, z+other.z);
    }

    Vec3 operator-(const Vec3& other) {
        return Vec3 (x-other.x, y-other.y, z-other.z);
    }

    Vec3 operator*(float other) {
        return Vec3 (x*other, y*other, z*other);
    }

    Vec3 operator/(float other) {
        return Vec3 (x/other, y/other, z/other);
    }

    float magnitude(){
        return this->dot(*this);
    }

    Vec3 normalise(){
        return Vec3(*this) / sqrt(this->magnitude());
    }
};

class Plane{
public:
    int num_points;
    Vec3 centroid, normal;


    Plane(int num_points, Vec3 centroid, Vec3 normal) 
        : num_points(num_points), centroid(centroid), normal(normal) {}

    Plane(){}
};

const float CRITICAL_INC = 5.0;
const float CRITICAL_DOT = std::sin(CRITICAL_INC);
const int PIXELS_PER_PLANE = 5;
constexpr int PIXELS_PER_PLANE2 = PIXELS_PER_PLANE * PIXELS_PER_PLANE;
const Vec3 up(0, 0, 1);

Map2D get_2d_map(Map3D& map3D){
    double start = std::clock();

    int num_planes_x = XS / PIXELS_PER_PLANE;
    int num_planes_y = YS / PIXELS_PER_PLANE;

    Map2D topHeightMap;
    Map2D bottomHeightMap;

    for (int i = 0; i < XS; i++) {
        for (int j = 0; j < YS; j++) {
            for (int k = 0; k < ZS; k++){
                if (map3D[i][j][k]) {
                    bottomHeightMap[i][j] = k;
                    break;
                }
               
            }
            for (int k = ZS - 1; k > -1; k--) {
                if (map3D[i][j][k]) {
                    topHeightMap[i][j] = k;
                    break;
                }
            }
        }
    }

            Vec3 centroid = point_sum / PIXELS_PER_PLANE2;

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

            xx /= PIXELS_PER_PLANE2;
            xy /= PIXELS_PER_PLANE2;
            xz /= PIXELS_PER_PLANE2;
            yy /= PIXELS_PER_PLANE2;
            yz /= PIXELS_PER_PLANE2;
            zz /= PIXELS_PER_PLANE2;

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
            if (std::abs(normal.dot(up)) < CRITICAL_DOT) {
                for (int i = 0; i < PIXELS_PER_PLANE; i++) {
                    for (int j = 0; j < PIXELS_PER_PLANE; j++) {
                        int x_index = plane_i * PIXELS_PER_PLANE + i;
                        int y_index = plane_j * PIXELS_PER_PLANE + j;

                        map2d[x_index][y_index] = 1.0;
                    }
                }
            }
        }
    }

    double end = std::clock();
    double time = (end - start)  / double(CLOCKS_PER_SEC);
    std::cout << "took " << time << "s" << std::endl;

    return map2d;
}

void sin_wave_terrain(Map3D& map3d) {
    for (float i = 0; i < XS; i += 2.5) {
        for (float j = 0; j < YS; j += 2.5) {
            float i2 = i * 2 * M_PI / 400;
            float j2 = j * 2 * M_PI / 400; 
            int k = (int) (24.5 * (sin(i2) + 1)); 
            map3d[i][j][k] = 1.0;
        }
    }
}

int main() {
    Map3D* map3D = new Map3D;
    sin_wave_terrain(*map3D);
    Map2D passable = get_2d_map(*map3D);

    free(map3D);
}
