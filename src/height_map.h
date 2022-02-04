class HeightMap {
public:
    int x_cm, y_cm;
    int pixel_x, pixel_y;

    std::vector<std::vector<float>> map;
    float pixel_size_cm;

    HeightMap(int x, int y, float pixel_size_cm);
    HeightMap(float pixel_size_cm);
    HeightMap();

    void add_map(std::vector<std::vector<float>> new_map);
    void add_point(float x, float y, float z);
    float get(float x, float y);
};