#include <vector>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <chrono>
#include <tuple>
#include <thread>

namespace py = pybind11;
typedef std::pair<int, int> Pair;
bool check_tuples(Pair pair) {
    return (std::get<0>(pair) > 5);
}

float array_sum(std::array<float, 10> lst){
    
    float sum = 0;

    for(unsigned int i = 0; i < lst.size(); i++){
        sum += lst[i];
    }

    return sum;
}

template <size_t ROW, size_t COL>
float array2d_sum(std::array<std::array<float, COL>, ROW> lst, Pair p1, Pair p2){
    
    float sum = 0;
    
    for(unsigned int i = 0; i < lst.size(); i++){
        for(unsigned int j = 0; j < lst[i].size(); j++){
	    sum += lst[i][j];
	}
    }

    return sum;
}

PYBIND11_MODULE(path_planner, module_handle) {
  
    module_handle.doc() = "Nova Rover PathPlanner C++ functions binded to Python3";
    module_handle.def("check_tuples", &check_tuples);
    module_handle.def("array_sum", &array_sum);
    module_handle.def("array2d_sum", &array2d_sum<10, 10>);
    module_handle.def("array2d_sum", &array2d_sum<11, 11>);
}

