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
    Vec3 operator=(const Vec3& other);
    Vec3 operator*(float other);
    Vec3 operator/(float other);
    float magnitude();
    Vec3 normalise();
};
