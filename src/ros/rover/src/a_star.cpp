// A C++ Program to implement A* Search Algorithm

#include <cmath>
#include <array>
#include <vector>
#include <chrono>
#include <cstring>
#include <iostream>
#include <queue>
#include <set>
#include <stack>
#include <tuple>
#include <chrono>
#include <iterator>
#include<list>
#include <fstream>
#include <string>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
using namespace std;

namespace py = pybind11;

/* These two parameters affect how much we value safety over distance.
   Increasing rover width will padd more widely around obstacles, and 
   the decay function will decay slower. Increasing the safety
   parameter means we take more care to avoid areas with any heuristic
   padding, even if their value is small. Weight impacts how heavily we
   weigh distance to the destination. Higher values run quicker but find
   a less optimal path. */
const float ROVER_WIDTH_CM = 50.0;
const float SAFETY_PARAMETER = 300.0;
const float WEIGHT = 1.0;

// Creating a shortcut for int, int pair type
typedef pair<int, int> Pair;
// Creating a shortcut for tuple<t> type
typedef tuple<double, int, int> Tuple;

// All possible neighbour points on an octile map
const array<Pair, 8> NEIGHBOURS = {{{0, 1}, {0, -1}, {1, 0}, {-1, 0},
									{1, 1}, {1, -1}, {-1, 1}, {-1, -1}}};

// A structure to hold the necessary parameters
struct cell {
	// Row and Column index of its parent
	Pair parent;
	// f = g + h
	float g;
	cell()
		: parent(-1, -1)
		, g(-1)
	{}
};

// A Utility Function to check whether given cell (row, col)
// is a valid cell or not.

bool isValid(const int cols, const int rows,
			const Pair& point)
{ // Returns true if row number and column number is in

	if (rows > 0 && cols > 0)
		return (point.first >= 0) && (point.first < rows)
			&& (point.second >= 0)
			&& (point.second < cols);

	return false;
}

bool isObstacle(const float grid_value)
{
    // Is this square specifically blocked by a physical obstacle?
	return grid_value == 1.0;
}

template <size_t ROW, size_t COL>
bool isSafe(const array<array<float, COL>, ROW>& grid,
				const Pair& point)
{
    /*is this square blocked by an obstacle or too close to one
	to be safe?*/
	return (isValid(COL, ROW, point)
		&& grid[point.first][point.second] < 1.0);
}

// A Utility Function to check whether destination cell has
// been reached or not
bool isDestination(const Pair& position, const Pair& dest)
{
	return position == dest;
}

float dist_squared(const Pair& p1, const Pair& p2)
{
	// Finds square of the Euclidean distance between two points - sqrt is expensive
	return pow((p1.first - p2.first), 2.0)
				+ pow((p1.second - p2.second), 2.0);
}

float heuristic(const Pair& p1, const Pair& p2) {
	// cheap approximation of Euclidean distance to save time
	// weird linear interpolation of the min and max distances 
	// https://www.flipcode.com/archives/Fast_Approximate_Distance_Functions.shtml
	// currently using manhattan distance because it is even faster
 	
    /*int min = std::min(abs(p1.first - p2.first), abs(p1.second - p2.second));
	int max = std::max(abs(p1.first - p2.first), abs(p1.second - p2.second));

    int approx = ( max * 1007 ) + ( min * 441 );

    return ((float) approx) / 1024;*/

	return abs(p1.first - p2.first) + abs(p1.second - p2.second);
}

float inverse_square_decay(float dist_sqrd, float scale_length_sqrd) 
{
	/* 
	Points too close are automatically impassable (values of 1.0 indicate obstacles,
	values of 1.1 are points too close to an obstacle to be safe. This allows us to
	distinguish between the two kinds of impassable points when padding)
	*/
	if (dist_sqrd < scale_length_sqrd) return 1.1;
	if (dist_sqrd > 9 * scale_length_sqrd) return 0.0; // outside a certain distance we don't care
	return (0.99 * scale_length_sqrd/ (dist_sqrd)); // inverse square decay scaled by scale length
}

template <size_t ROW, size_t COL>
void optimise_padding_area(const array<array<float, COL>, ROW>& grid, 
							int& min_x, int& max_x, int& min_y, int& max_y)
{
	/*
	Looks for other obstacle points around the start point in all 4 directions. If there
	is a point in one direction, we only need to pad as far as half-way to that point
	*/

	int x = (min_x + max_x) / 2;
	int y = (min_y + max_y) / 2;

	// bounding min and max values by dimensions of map
	min_x = max(min_x, 0); 
	min_y = max(min_y, 0);
	max_x = min(max_x, (int) COL);
	max_y = min(max_y, (int) ROW);

	for (int i = y - 1; i >= min_y; i--) {
		if (isObstacle(grid[i][x])) {
			min_y = ceil((i + y)/ 2);
		}
	}

	for (int i = y + 1; i <= max_y; i++) {
		if (isObstacle(grid[i][x])) {
			max_y = floor((i + y) / 2);
		}
	}

	for (int j = x - 1; j >= min_x; j--) {
		if (isObstacle(grid[y][j])) {
			min_x = ceil((j + x) / 2);
		}
	}

	for (int j = x + 1; j <= max_x; j++) {
		if (isObstacle(grid[y][j])) {
			max_x = floor((j + x) / 2);
		}
	}
}

template <size_t ROW, size_t COL> 
void pad_point(array<array<float, COL>, ROW>& grid, const Pair& start_pt, float scale_length_pixels)
{
	int y = start_pt.first, x = start_pt.second;
	int min_x = x - 3 * scale_length_pixels, min_y = y - 3 * scale_length_pixels;
	int max_x = x + 3 * scale_length_pixels, max_y = y + 3 * scale_length_pixels;

	optimise_padding_area(grid, min_x, max_x, min_y, max_y);

	for (int k = min_y; k <= max_y; k++) {
		for (int l = min_x; l <= max_x; l++) {
			Pair there(k, l);
			if (isValid(COL, ROW, there)) {
				float grid_val = grid[k][l];
				float new_val = inverse_square_decay(dist_squared(there, start_pt),
									pow(scale_length_pixels, 2.0));

				if (new_val > grid_val && !isObstacle(grid_val)) {
					grid[k][l] = new_val;
				}
			}
		}
	}
}

template <size_t ROW, size_t COL>
void precompute_padding_values(array<array<float, COL>, ROW>& grid, 
									float grid_resolution_cm) 
{
	/* Loops through the map, locates every obstacle tile, and maps
	 out a region around it with a breadth-first search where the 
	 rover cannot travel. Weights tiles with an inverse-square decay
	 by there distsance to the obstacle to discourage the rover from
	 coming too close */
	double scale_length_pixels = ROVER_WIDTH_CM / grid_resolution_cm;

	auto start = chrono::high_resolution_clock::now();
	for (uint i = 0; i < ROW; i++) {
		for (uint j = 0; j < COL; j++) {
			if (isObstacle(grid[i][j])) {
				// we have encountered an obstacle! Pad around it.
				Pair here(i, j);
				pad_point(grid, here, scale_length_pixels);
			}
		}
	}

	auto end = chrono::high_resolution_clock::now();
	auto duration = chrono::duration_cast<std::chrono::microseconds>(end - start);
	
	cout << "padding took " << duration.count() << " microseconds" << endl;
}

// A Utility Function to trace the path from the source to
// destination
template <size_t ROW, size_t COL>
vector<Pair> tracePath(array<array<cell, COL>, ROW>& cellDetails, const Pair& dest){
	printf("\nThe Path is ");

	stack<Pair> backwards_path;

	int row = dest.second;
	int col = dest.second;
	Pair next_node = dest;
	do {
		backwards_path.push(next_node);
		next_node = cellDetails[row][col].parent;
		row = next_node.first;
		col = next_node.second;
	} while (cellDetails[row][col].parent != next_node);

	backwards_path.emplace(row, col);
	vector<Pair> path;
	while (!backwards_path.empty()){
		Pair p = backwards_path.top();
		backwards_path.pop();
		path.push_back(p);
	}

	return path;
}

// A Function to find the intest path between a given
// source cell to a destination cell according to A* Search
// Algorithm

template <size_t ROW, size_t COL>
vector<Pair> aStarSearch(array<array<float, COL>, ROW>& grid,
				const Pair& src, const Pair& dest, const float grid_resolution_m)
{
	// timer to check performance
	auto start = chrono::high_resolution_clock::now();
	const float grid_resolution_cm = grid_resolution_m * 100;
	// assign heuristic values according to distance to nearest obstacle
	precompute_padding_values(grid, grid_resolution_cm);

	// If the source is out of range
	if (!isSafe(grid, src)) {
		printf("Invalid or unsafe starting point\n");
		return vector<Pair> {{src}};
	}

	// If the destination is out of range
	if (!isSafe(grid, dest)) {
		printf("Invalid or unsafe destination point\n");
		return vector<Pair> {{src}};
	}

	// If the destination cell is the same as source cell
	if (isDestination(src, dest)) {
		printf("We are already at the destination\n");
		return vector<Pair> {{src}};
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
	cellDetails[i][j].g = 0.0;
	cellDetails[i][j].parent = { i, j };

	/*
	Create an open list having information as-
	<f, <i, j>>
	where f = g + h,
	and i, j are the row and column index of that cell
	Note that 0 <= i <= ROW-1 & 0 <= j <= COL-1
	This open list is implemented as a set of tuple.*/
	std::priority_queue<Tuple, std::vector<Tuple>,
						std::greater<Tuple>> openList;

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
				if(add_x == 0 && add_y == 0) continue;

				Pair neighbour(i + add_x, j + add_y);
				// Only process this cell if this is a valid
				// one
				if (isSafe(grid, neighbour) && !closedList[neighbour.first][neighbour.second]) {
					// If the destination cell is the same
					// as the current successor
					if (isDestination(neighbour, dest)) { // Set the Parent of the destination cell
						cellDetails[neighbour.first][neighbour.second].parent = { i, j };
						printf("The destination cell is found\n");

						auto stop = chrono::high_resolution_clock::now();
						auto duration = chrono::duration_cast<std::chrono::microseconds>(stop - start);
					
						// To get the value of duration use the count()
						// member function on the duration object
						cout << "\ntook " << duration.count() << " microseconds" << endl;
						
						return tracePath(cellDetails, dest);
					}
					
					float gNew, hNew, fNew;
					// additional distance to next point
					float g_diff = (add_y == 0 || add_x == 0) ? 1.0 : sqrt(2.0);
					gNew = cellDetails[i][j].g + g_diff;
					hNew = grid[neighbour.first][neighbour.second] * SAFETY_PARAMETER + heuristic(neighbour, dest) * WEIGHT;
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
					if (cellDetails[neighbour.first][neighbour.second].g == -1
						|| cellDetails[neighbour.first][neighbour.second].g > gNew) {
						openList.emplace(fNew, neighbour.first, neighbour.second);

						// Update the details of this
						// cell
						cellDetails[neighbour.first]
								[neighbour.second]
									.g
							= gNew;
						
						cellDetails[neighbour.first]
								[neighbour.second]
									.parent
							= { i, j };
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

	return vector<Pair> {{src}};
}

// Driver program to test above function
int main()
{
	/* Description of the Grid-
	1--> The cell is blocked
	0--> The cell is not blocked */
	array<array<float, 200>, 200> grid;
	grid.fill({});

	for (int i = 0; i < 100; i++) {
		grid[i][50] = 1.0;
	}
	// Source is the left-most bottom-most corner
	Pair src(0, 0);

	// Destination is the left-most top-most corner
	Pair dest(199, 199);
	vector<Pair> path = aStarSearch(grid, src, dest, 2.5);

	return 0;
}

PYBIND11_MODULE(a_star, module_handle) {
    module_handle.doc() = "Nova Rover A* C++ search algorithm binded to Python3";
    module_handle.def("a_star", &aStarSearch<800, 800>); // 2.5 cm resolution
    module_handle.def("a_star", &aStarSearch<400, 400>); // 5 cm resolution
    module_handle.def("a_star", &aStarSearch<200, 200>); // 10 cm resolution
    module_handle.def("a_star", &aStarSearch<100, 100>); // 20 cm resolution
    module_handle.def("a_star", &aStarSearch<150, 150>); // 4 cm resolution on 6m map
	
}

