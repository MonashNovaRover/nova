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
const float SAFETY_PARAMETER = 200.0;
const float WEIGHT = 1.0;

// Creating a shortcut for int, int pair type
typedef pair<int, int> Pair;
// Creating a shortcut for tuple<int, int, int> type
typedef tuple<double, int, int> Tuple;

// All possible neighbour points on an octile map
const array<Pair, 8> NEIGHBOURS = {{{0, 1}, {0, -1}, {1, 0}, {-1, 0},
									{1, 1}, {1, -1}, {-1, 1}, {-1, -1}}};

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

template <size_t ROW, size_t COL>
bool isUnBlocked(const array<array<float, COL>, ROW>& grid,
				const Pair& point)
{
    /*A Utility Function to check whether the given cell is
     blocked or not. Returns true if the cell is not blocked else false.
	 Open cells have values less than 1 otherwise they are blocked*/
	return (isValid(COL, ROW, point)
		&& grid[point.first][point.second] < 1.0);
}

// A Utility Function to check whether destination cell has
// been reached or not
bool isDestination(const Pair& position, const Pair& dest)
{
	return position == dest;
}

/*template <size_t ROW, size_t COL>
void print_grid(const array<array<float, COL>, ROW>& grid) {
	ifstream f;
    string buf;
    f.open("data.txt");
    for(int i=0;i<ROW;i++)
    {
		for (int j = 0; j < COL; j++) {

        	f << grid[i][j] << "\t";
		}
		f << endl;

    }
    f.close();
}*/

float dist(const Pair& p1, const Pair& p2)
{
	// Finds Euclidean distance between two points
	return sqrt(pow((p1.first - p2.first), 2.0)
				+ pow((p1.second - p2.second), 2.0));
}

float inverse_square_decay(float dist, float scale_length) 
{
	/* points too close are automatically impassable (use 1.1 to distinguish
	from "1st generation" obstacles which are exactly 1.0)*/
	if (dist < scale_length) return 1.1;
	if (dist > 3 * scale_length) return 0.0; // outside a certain distance we don't care
	return (0.99 / pow(dist / scale_length, 2.0)); // inverse square decay scaled by scale length
}

template <size_t ROW, size_t COL>
array<array<float, COL>, ROW> precompute_padding_values(array<array<float, COL>, ROW>& grid, 
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
			if (grid[i][j] == 1.0f) {
				// we have encountered an obstacle! Pad around it.
				Pair here(i, j);
				int length = 3 * scale_length_pixels;
				for (int k = -length; k <= length; k++) {
					for (int l = -length; l <= length; l++) {
						Pair there(i + k, j + l);
						if (isValid(COL, ROW, there)) {
							float grid_val = grid[there.first][there.second];
							float new_val = inverse_square_decay(dist(there, here),
												scale_length_pixels);

							if (new_val > grid_val && grid_val != 1.0f) {
								grid[there.first][there.second] = new_val;
							}
						}
					}
				}
			}
		}
	}

	auto end = chrono::high_resolution_clock::now();
	auto duration = chrono::duration_cast<std::chrono::microseconds>(end - start);
	
	cout << "took " << duration.count() << " microseconds" << endl;
	//print_grid(grid);
	return grid;
}

// A Utility Function to trace the path from the source to
// destination
template <size_t ROW, size_t COL>
vector<Pair> tracePath(const array<array<cell, COL>, ROW>& cellDetails, const Pair& dest){
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

// A Function to find the shortest path between a given
// source cell to a destination cell according to A* Search
// Algorithm

template <size_t ROW, size_t COL>
vector<Pair> aStarSearch(array<array<float, COL>, ROW>& grid,
				const Pair& src, const Pair& dest, const float grid_resolution_cm)
{
	// timer to check performance
	auto start = chrono::high_resolution_clock::now();

	// assign heuristic values according to distance to obstacles

	precompute_padding_values(grid, grid_resolution_cm);

    // pad the map (ignoring for now)
    //array<array<float, COL>, ROW> grid = maxPool(grid, pad_size=10);

	// If the source is out of range
	if (!isUnBlocked(grid, src)) {
		printf("Source is invalid\n");
		return vector<Pair> {{src}};
	}

	// If the destination is out of range
	if (!isUnBlocked(grid, src)) {
		printf("Destination is invalid\n");
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
				if (isUnBlocked(grid, neighbour)) {
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
					// If the successor is already on the
					// closed list or if it is blocked, then
					// ignore it. Else do the following
					else if (!closedList[neighbour.first]
										[neighbour.second]
							&& isUnBlocked(grid,
											neighbour)) {
						double gNew, hNew, fNew;
						// additional distance to next point
						double g_diff = (add_y == 0 || add_x == 0) ? 1.0 : sqrt(2.0);
						gNew = cellDetails[i][j].g + g_diff;
						hNew = grid[neighbour.first][neighbour.second] * SAFETY_PARAMETER + dist(neighbour, dest) * WEIGHT;
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
}

