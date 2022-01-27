#include <vector>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <chrono>
#include <thread>

namespace py = pybind11;

float test() {
  return 2.5;
}


float array_sum(std::vector<float> lst){
    
    float sum = 0;

    for(unsigned int i = 0; i < lst.size(); i++){
        sum += lst[i];
    }

    return sum;
}


float array2d_sum(std::vector<std::vector<float>> lst){
    
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
    module_handle.def("test", &test);
    module_handle.def("array_sum", &array_sum);
    module_handle.def("array2d_sum", &array2d_sum);
    
}

