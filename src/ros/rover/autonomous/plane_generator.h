class Vec3{
public:
    float x, y, z;
    Vec3();
    Vec3(float x, float y, float z);
    Vec3(const Vec3& other);

    float dot(const Vec3& other);
    Vec3 cross(const Vec3& other);
    Vec3 operator+(const Vec3& other);
    Vec3 operator-(const Vec3& other);
    Vec3 operator*(float other);
    Vec3 operator/(float other);
    float magnitude();
    Vec3 normalise();
};

class Plane{
public:
    std::vector<Vec3> points;
    Vec3 centroid, normal;

    Plane(std::vector<Vec3> points, Vec3 centroid, Vec3 normal);
    Plane();
};

class PlaneGenerator {
public:
    int num_planes_x, num_planes_y, plane_x_pixels, plane_y_pixels, pixels_per_plane;
    float pixel_size_cm;
    std::vector<std::vector<Plane>> planes;
    HeightMap height_map;

    PlaneGenerator(int num_planes_x, int num_planes_y);
    void add_height_map(HeightMap height_map);
    void fit_planes(HeightMap h);
    void fit_planes();

private:
    float get(int x, int y);
};
