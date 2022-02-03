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


// Creating a shortcut for int, int pair type
typedef pair<int, int> Pair;
// Creating a shortcut for tuple<int, int, int> type
typedef tuple<double, int, int> Tuple;

// A structure to hold the necessary parameters
struct cell {
	// Row and Column index of its parent
	Pair parent;
	// f = g + h
	double f, g, h;
	cell()
		: parent(-1, -1)
		, f(-1)
		, g(-1)
		, h(-1)
	{
	}
};

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

// A Utility Function to check whether given cell (row, col)
// is a valid cell or not.
template <size_t ROW, size_t COL>
bool isValid(const array<array<float, COL>, ROW>& grid,
			const Pair& point)
{ // Returns true if row number and column number is in

	if (ROW > 0 && COL > 0)
		return (point.first >= 0) && (point.first < ROW)
			&& (point.second >= 0)
			&& (point.second < COL);

	return false;
}

template <size_t ROW, size_t COL>
bool isUnBlocked(const array<array<float, COL>, ROW>& grid,
				const Pair& point)
{
    // A Utility Function to check whether the given cell is
    // blocked or not
	// Returns true if the cell is not blocked else false
	return isValid(grid, point)
		&& grid[point.first][point.second] == 1;
}

// A Utility Function to check whether destination cell has
// been reached or not
bool isDestination(const Pair& position, const Pair& dest)
{
	return position == dest;
}

// A Utility Function to calculate the 'h' heuristics.
double calculateHValue(const Pair& src, const Pair& dest)
{
	// h is estimated with the two points distance formula
	return sqrt(pow((src.first - dest.first), 2.0)
				+ pow((src.second - dest.second), 2.0));
}

// A Utility Function to trace the path from the source to
// destination
template <size_t ROW, size_t COL>
array<Pair> tracePath(const array<array<cell, COL>, ROW>& cellDetails, const Pair& dest){
	printf("\nThe Path is ");

	stack<Pair> Path;

	int row = dest.second;
	int col = dest.second;
	Pair next_node = cellDetails[row][col].parent;
	do {
		Path.push(next_node);
		next_node = cellDetails[row][col].parent;
		row = next_node.first;
		col = next_node.second;
	} while (cellDetails[row][col].parent != next_node);

	Path.emplace(row, col);
	while (!Path.empty()) {
		Pair p = Path.top();
		Path.pop();
		printf("-> (%d,%d) ", p.first, p.second);
	}
}

// A Function to find the shortest path between a given
// source cell to a destination cell according to A* Search
// Algorithm

template <size_t ROW, size_t COL>
void aStarSearch(array<array<float, COL>, ROW> grid,
				const Pair& src, const Pair& dest)
{
	// timer to check performance
	auto start = chrono::high_resolution_clock::now();

    // pad the map
    array<array<float, COL>, ROW> grid = maxPool(grid, pad_size=10);

	// If the source is out of range
	if (!isValid(grid, src)) {
		printf("Source is invalid\n");
		return;
	}

	// If the destination is out of range
	if (!isValid(grid, dest)) {
		printf("Destination is invalid\n");
		return;
	}

	// Either the source or the destination is blocked
	if (!isUnBlocked(grid, src)
		|| !isUnBlocked(grid, dest)) {
		printf("Source or the destination is blocked\n");
		return;
	}

	// If the destination cell is the same as source cell
	if (isDestination(src, dest)) {
		printf("We are already at the destination\n");
		return;
	}

	// Create a closed list and initialise it to false which
	// means that no cell has been included yet This closed
	// list is implemented as a boolean 2D array
	bool closedList[ROW][COL];
	memset(closedList, false, sizeof(closedList));

	// Declare a 2D array of structure to hold the details
	// of that cell
	array<array<cell, COL>, ROW> cellDetails;

	int i, j;
	// Initialising the parameters of the starting node
	i = src.first, j = src.second;
	cellDetails[i][j].f = 0.0;
	cellDetails[i][j].g = 0.0;
	cellDetails[i][j].h = 0.0;
	cellDetails[i][j].parent = { i, j };

	/*
	Create an open list having information as-
	<f, <i, j>>
	where f = g + h,
	and i, j are the row and column index of that cell
	Note that 0 <= i <= ROW-1 & 0 <= j <= COL-1
	This open list is implemented as a set of tuple.*/
	std::priority_queue<Tuple, std::vector<Tuple>,
						std::greater<Tuple> >
		openList;

	// Put the starting cell on the open list and set its
	// 'f' as 0
	openList.emplace(0.0, i, j);

	// We set this boolean value as false as initially
	// the destination is not reached.
	while (!openList.empty()) {
		const Tuple& p = openList.top();
		// Add this vertex to the closed list
		i = get<1>(p); // second element of tupla
		j = get<2>(p); // third element of tupla

		// Remove this vertex from the open list
		openList.pop();
		closedList[i][j] = true;
		/*
				Generating all the 8 successor of this cell
						N.W N N.E
						\ | /
						\ | /
						W----Cell----E
								/ | \
						/ | \
						S.W S S.E

				Cell-->Popped Cell (i, j)
				N --> North	 (i-1, j)
				S --> South	 (i+1, j)
				E --> East	 (i, j+1)
				W --> West		 (i, j-1)
				N.E--> North-East (i-1, j+1)
				N.W--> North-West (i-1, j-1)
				S.E--> South-East (i+1, j+1)
				S.W--> South-West (i+1, j-1)
		*/
		for (int add_x = -1; add_x <= 1; add_x++) {
			for (int add_y = -1; add_y <= 1; add_y++) {
				Pair neighbour(i + add_x, j + add_y);
				// Only process this cell if this is a valid
				// one
				if (isValid(grid, neighbour)) {
					// If the destination cell is the same
					// as the current successor
					if (isDestination(neighbour, dest)) { // Set the Parent of the destination cell
						cellDetails[neighbour.first][neighbour.second].parent = { i, j };
						printf("The destination cell is found\n");
						tracePath(cellDetails, dest);

						auto stop = chrono::high_resolution_clock::now();
						auto duration = chrono::duration_cast<std::chrono::microseconds>(stop - start);
					
						// To get the value of duration use the count()
						// member function on the duration object
						cout << "\ntook " << duration.count() << " microseconds" << endl;
						return;
					}
					// If the successor is already on the
					// closed list or if it is blocked, then
					// ignore it. Else do the following
					else if (!closedList[neighbour.first]
										[neighbour.second]
							&& isUnBlocked(grid,
											neighbour)) {
						double gNew, hNew, fNew;
						gNew = cellDetails[i][j].g + 1.0;
						hNew = calculateHValue(neighbour,
											dest);
						fNew = gNew + hNew;

						// If it isn’t on the open list, add
						// it to the open list. Make the
						// current square the parent of this
						// square. Record the f, g, and h
						// costs of the square cell
						//			 OR
						// If it is on the open list
						// already, check to see if this
						// path to that square is better,
						// using 'f' cost as the measure.
						if (cellDetails[neighbour.first][neighbour.second].f == -1
							|| cellDetails[neighbour.first][neighbour.second].f > fNew) {
							openList.emplace(fNew, neighbour.first, neighbour.second);

							// Update the details of this
							// cell
							cellDetails[neighbour.first]
									[neighbour.second]
										.g
								= gNew;
							cellDetails[neighbour.first]
									[neighbour.second]
										.h
								= hNew;
							cellDetails[neighbour.first]
									[neighbour.second]
										.f
								= fNew;
							cellDetails[neighbour.first]
									[neighbour.second]
										.parent
								= { i, j };
						}
					}
				}
			}
		}
	}

	// When the destination cell is not found and the open
	// list is empty, then we conclude that we failed to
	// reach the destiantion cell. This may happen when the
	// there is no way to destination cell (due to
	// blockages)



	printf("Failed to find the Destination Cell\n");
}

// Driver program to test above function
int main()
{
	/* Description of the Grid-
	1--> The cell is not blocked
	0--> The cell is blocked */
	array<array<float, 10>, 9> grid{
		{ { { 1., 0., 1., 1., 1., 1., 0., 1., 1., 1. } },
		{ { 1., 1., 1., 0., 1., 1., 1., 0., 1., 1. } },
		{ { 1., 1., 1., 0., 1., 1., 0., 1., 0., 1. } },
		{ { 0., 0., 1., 0., 1., 0., 0., 0., 0., 1. } },
		{ { 1., 1., 1., 0., 1., 1., 1., 0., 1., 0. } },
		{ { 1., 0., 1., 1., 1., 1., 0., 1., 0., 0. } },
		{ { 1., 0., 0., 0., 0., 1., 0., 0., 0., 1. } },
		{ { 1., 0., 1., 1., 1., 1., 0., 1., 1., 1. } },
		{ { 1., 1., 1., 0., 0., 0., 1., 0., 0., 1. } } }
	};

	// Source is the left-most bottom-most corner
	Pair src(7, 9);

	// Destination is the left-most top-most corner
	Pair dest(0, 0);
	aStarSearch(grid, src, dest);
	return 0;
}

PYBIND11_MODULE(a_star, module_handle) {
    module_handle.doc() = "Nova Rover A* C++ search algorithm binded to Python3";
    module_handle.def("a_star", &aStarSearch<800, 800>); // 2.5 cm resolution
    module_handle.def("a_star", &aStarSearch<400, 400>); // 5 cm resolution
    module_handle.def("a_star", &aStarSearch<200, 200>); // 10 cm resolution
    module_handle.def("a_star", &aStarSearch<100, 100>); // 20 cm resolution
}

