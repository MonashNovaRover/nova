/*
 * A fast obstacle padding - computes 2D convolutions on arrays
 *
 * */

// A C++ Program to implement A* Search Algorithm
#include "math.h"
#include <array>
#include <chrono>
#include <cstring>
#include <iostream>
#include <queue>
#include <set>
#include <stack>
#include <tuple>
#include <chrono>
#include<list>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
using namespace std;

namespace py = pybind11;

template <size_t ROW, size_t COL>
array<array<float, COL>, ROW> maxPool(array<array<float, COL>, ROW> grid, const int pad_size{
	/*
	 * Gets the Max values for each pad_size^2 window of a ROW*COL grid
	 * For use in basic 2D map padding.
	 * Future improvements: only accept values within a circle (rover turning radius) instead of a square
	 * Only accept
	 * */

    // initialize a 2D array of zeros
	array<array<float, COL>, ROW> max_pool;
    conv.fill({});

	for (int x = 0; x < COL; x++){
		for int y = 0; y < ROW; y++){
            // we are doing a "padding=None" style of max_pool
            int x_start = std::min(std::max(0, x - pad_size), COL - 1);
            int x_end   = std::min(std::max(0, x + pad_size), COL - 1);
            int y_start = std::min(std::max(0, y - pad_size), ROW - 1);
            int y_end   = std::min(std::max(0, y + pad_size), ROW - 1);

            // probably a statistically faster method - maybe searching from the middle and moving outward in a spiral?
            for (int x_pad = x_start; x_pad < x_end; x += 1){
                for (int y_pad = y_start; y_pad < y_end; x += 1){
                    if (grid[x_pad][y_pad] != 0.0){
                        max_pool[x][y] == 1.0;
                        break;
                    }
                }
            }
		}
	}
    return max_pool;
}
