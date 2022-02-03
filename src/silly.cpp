#include <iostream>
#include <array>
void nothing(std::array<std::array<int, 3>, 3>& arr);
int main() {
    std::array<std::array<int, 3>, 3> arr = {{{0, 0, 0}, {0, 0, 0}, {0, 0, 0}}};

    nothing(arr);
    std::cout << arr[1][2] << std::endl;
}

void nothing(std::array<std::array<int, 3>, 3>& arr) {

    arr[1][2] = 1;
}
