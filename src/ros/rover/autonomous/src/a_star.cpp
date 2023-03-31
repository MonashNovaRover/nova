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
const float TERRAIN_IMPORTANCE = 300; // How much we value smooth terrain over distance
const float SAFETY_FACTOR = 1.6; // Factor by which we multiply rover radius to pad.
const float WEIGHT = 1.0; // weight of heuristic for A*
const float SOURCE_OBSTACLE_CLEARANCE_RADIUS_M = 0.6; // distance to which we remove obstacles at src
const float DEST_OBSTACLE_CLEARANCE_RADIUS_M = 0.3; // distance to which we remove obstacles at dest
const int CRITICAL_PATH_LEN = 1;
const float OBSTACLE_VALUE = 1.0;
const float HEIGHT_OBSTACLE_VALUE = 1.1;
const float PLANE_PADDING_DISTANCE_FRACTION = 1.0;
const float C_INF = 1e12;
const float NEAREST_POINT_DIST_WEIGHT = 0.5;
const float STRING_PULL_OBSTACLE_VAL = 0.5;
float PADDING_DIST_M = 0; 

// implementation of type-safe enum with bitwise operators taken from:
// https://wiggling-bits.net/using-enum-classes-as-type-safe-bitmasks/
enum class Status : unsigned {
	/*
	Enum returned to python detailing the outcome of A* in finding a path.
	All values are powers of 2 so that each outcome is represented by the state
	of a single bit in the returned value.
	*/
	A_STAR_SUCCESS = 0,
	A_STAR_START_OBSTACLE = 1,
	A_STAR_DEST_OBSTACLE = 2,
	A_STAR_NO_PATH = 4,
	A_STAR_CRITICAL_NO_PATH = 8
};

Status& operator |=(Status &lhs, Status rhs)
{
    lhs = static_cast<Status> (
        static_cast<std::underlying_type<Status>::type>(lhs) |
        static_cast<std::underlying_type<Status>::type>(rhs)           
    );

    return lhs;
}

// Creating a shortcut for int, int pair type
typedef pair<int, int> Pair;
// Creating a shortcut for tuple<t> type
typedef tuple<double, int, int> Tuple;

// All possible neighbour points on an octile map
const array<Pair, 8> BRANCHES = {{{0, 1}, {0, -1}, {1, 0}, {-1, 0},
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
	return grid_value == OBSTACLE_VALUE || grid_value == HEIGHT_OBSTACLE_VALUE;
}

template <size_t ROW, size_t COL>
bool isSafe(const array<array<float, COL>, ROW>& grid,
				const Pair& point)
{
    /*is this square blocked by an obstacle or too close to one
	to be safe?*/
	if (isValid(COL, ROW, point)) 
		return grid[point.first][point.second] < OBSTACLE_VALUE;
    return true; // destination is not valid (ie not in map), so it is not in an obstacle to our knowledge
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

	return std::sqrt(dist_squared(p1, p2));

	//return abs(p1.first - p2.first) + abs(p1.second - p2.second);
}

float padding_value(float dist_sqrd, float padding_width_sqrd) 
{
	/* 
	Points too close are automatically impassable (values of 1.0 indicate obstacles,
	values of 1.2 are points too close to an obstacle to be safe. This allows us to
	distinguish between the two kinds of impassable points when padding)
	*/
	if (dist_sqrd < padding_width_sqrd) return 1.2 * OBSTACLE_VALUE;
    if (dist_sqrd < 2 * padding_width_sqrd) return 0.0;
	return 0;
}

template <size_t ROW, size_t COL>
void optimise_padding_area(const array<array<float, COL>, ROW>& grid, 
							int& x, int& y, int& min_x, int& max_x, int& min_y, int& max_y)
{
	/*
	Looks for other obstacle points around the start point in all 4 directions. If there
	is a point in one direction, we only need to pad as far as half-way to that point
	*/

	// bounding min and max values by dimensions of map
	min_x = max(min_x, 0); 
	min_y = max(min_y, 0);
	max_x = min(max_x, (int) COL);
	max_y = min(max_y, (int) ROW);

	for (int i = y - 1; i >= min_y; i--) {
		if (isObstacle(grid[i][x])) {
			min_y = floor((i + y)/ 2);
		}
	}

	for (int i = y + 1; i <= max_y; i++) {
		if (isObstacle(grid[i][x])) {
			max_y = ceil((i + y) / 2);
		}
	}

	for (int j = x - 1; j >= min_x; j--) {
		if (isObstacle(grid[y][j])) {
			min_x = floor((j + x) / 2);
		}
	}

	for (int j = x + 1; j <= max_x; j++) {
		if (isObstacle(grid[y][j])) {
			max_x = ceil((j + x) / 2);
		}
	}
}

template<size_t ROW, size_t COL>
int count_adjacent_obstacles(const array<array<float, COL>, ROW>& grid, int& x, int& y){
	/*
	Count the number of obstacle tiles ( == 1) directly adjacent to an x,y coordinate
	*/

	int adjacent = 0;

	for (int dx = -1; dx <= 1; dx += 2){
		if (x + dx < 0 || (size_t) (x + dx) >= COL) continue;
		if (grid[y][x + dx] == OBSTACLE_VALUE || grid[y][x + dx] == HEIGHT_OBSTACLE_VALUE) adjacent++;
	}

	for (int dy = -1; dy <= 1; dy += 2){
		if (y + dy < 0 || (size_t) (y + dy) >= ROW) continue;
		if (grid[y + dy][x] == OBSTACLE_VALUE || grid[y + dy][x] == HEIGHT_OBSTACLE_VALUE) adjacent++;
	}

	return adjacent;
}


template <size_t ROW, size_t COL> 
void pad_point(array<array<float, COL>, ROW>& grid, const Pair& start_pt, float padding_width_pixels)
{
	int y = start_pt.first, x = start_pt.second;
	int min_x = x - 1.3 * padding_width_pixels, min_y = y - 1.3 * padding_width_pixels;
	int max_x = x + 1.3 * padding_width_pixels, max_y = y + 1.3 * padding_width_pixels;

	// if the obstale is an isolated point - it is likely a phantom obstacle - don't pad around it
	if (count_adjacent_obstacles(grid, x, y) <= 1) return;

	optimise_padding_area(grid, x, y, min_x, max_x, min_y, max_y);

	for (int k = min_y; k <= max_y; k++) {
		for (int l = min_x; l <= max_x; l++) {
			Pair there(k, l);
			if (isValid(COL, ROW, there)) {
				float grid_val = grid[k][l];
				float new_val = padding_value(dist_squared(there, start_pt),
									pow(padding_width_pixels, 2.0));

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
	double padding_width_pixels = PADDING_DIST_M * 100 / grid_resolution_cm;

	for (uint i = 0; i < ROW; i++) {
		for (uint j = 0; j < COL; j++) {
			if (isObstacle(grid[i][j])) {
				// we have encountered an obstacle! Pad around it.
				Pair here(i, j);
                // padding plane obstacles half as far
                if (grid[i][j] == OBSTACLE_VALUE) pad_point(grid, here, PLANE_PADDING_DISTANCE_FRACTION * padding_width_pixels);
                // padding height obstacles normal distance 
                else if (grid[i][j] == HEIGHT_OBSTACLE_VALUE) pad_point(grid, here, padding_width_pixels);
			}
		}
	}
}

template <size_t ROW, size_t COL> 
void add_obstacle_border(array<array<float, COL>, ROW>& grid)
{
	/* Adds a border of obstacles around the map to prevent the rover from
	 driving off the map */
	uint j = 0;
	for (uint i = 0; i < ROW; i++) {
		grid[i][j] = OBSTACLE_VALUE;
	}
	j = COL - 1;
	for (uint i = 0; i < ROW; i++) {
		grid[i][j] = OBSTACLE_VALUE;
	}

	uint i = 0;
	for (uint j = 0; j < COL; j++) {
		grid[i][j] = OBSTACLE_VALUE;
	}
	i = ROW - 1;
	for (uint i = 0; i < ROW; i++) {
		grid[i][j] = OBSTACLE_VALUE;
	}
}

// A Utility Function to trace the path from the source to
// destination
template <size_t ROW, size_t COL>
vector<Pair> tracePath(array<array<cell, COL>, ROW>& cellDetails, const Pair& dest){
	stack<Pair> backwards_path;

	int row = dest.second;
	int col = dest.first;
	Pair next_node = dest;
	do {
		backwards_path.push(next_node);
		next_node = cellDetails[col][row].parent;
		row = next_node.second;
		col = next_node.first;
	} while (cellDetails[col][row].parent != next_node);

	backwards_path.emplace(col, row);
	vector<Pair> path;
	while (!backwards_path.empty()){
		Pair p = backwards_path.top();
		backwards_path.pop();
		path.push_back(p);
	}

	return path;
}

template <size_t ROW, size_t COL>
void clear_obstacles_from_location(array<array<float, COL>, ROW>& grid, const Pair& point, const int cutting_distance) {
	/*
	Checks area around a point, and sets any obstacle values in that area to 0.99 rather than 1.
	Prevents hard failure if the rover thinks its current pose or its destination is inside an obstacle.
	:param grid: reference to the grid being used for A*, to remove obstacles from
	:param point: point around which to remove obstacles
	:param cutting_distance: (octile) pixel distance around which to clear obstacles
	*/

	int x = point.first, y = point.second;
	for (int i = max(0, x - cutting_distance); i <= min((int) COL, x + cutting_distance); i++) {
		for (int j = max(0, y - cutting_distance); j <= min((int) ROW, y + cutting_distance); j++){
			if (grid[i][j] >= OBSTACLE_VALUE) grid[i][j] = 0.99 * OBSTACLE_VALUE;
		}
	}
}

template <size_t ROW, size_t COL>
bool obstacle_between(array<array<float, COL>, ROW>& grid, Pair p1, Pair p2, int num_points){
    /*
    Interpolates between two points, checking from terrain above a specific obstacle value.
    :return: true if an obstacle is found between p1 and p2, otherwise false
    */
    int x1 = p1.first, y1 = p1.second;
    int x2 = p2.first, y2 = p2.second;

    double dx = (double) (x2 - x1) / num_points;
    double dy = (double) (y2 - y1) / num_points;

    double x = x1, y = y1;

    for (int i = 0; i < num_points; i++){
		x += dx;
		y += dy;
		if (grid[(int) x][(int) y] > STRING_PULL_OBSTACLE_VAL){
			return true;
		}
    }
    return false;
}

template <size_t ROW, size_t COL>
void string_pull_from_start(array<array<float, COL>, ROW>& grid, vector<Pair>& path){
    /*
    Reduce the path to a substring of itself beginning at the first point necessitated by a string pull
    */
    Pair start = path[0];
    for (size_t i = 0; i < path.size(); i++){
		Pair this_point = path[i+1];
		int num_points = (int) heuristic(start, this_point);
        if (obstacle_between(grid, start, this_point, num_points) || num_points > 50){
            path = {path.begin() + i, path.end()};
			path[0] = start;
            return;
        }
    }
}

vector<Pair> construct_return_val(vector<Pair> path, Status status) {
	//string_pull_from_start(grid, path);
	path.push_back(Pair((int) status, -1));

	return path;
}

// A Function to find the intest path between a given
// source cell to a destination cell according to A* Search
// Algorithm

template <size_t ROW, size_t COL>
vector<Pair> aStarSearch(array<array<float, COL>, ROW>& grid,
				const Pair& src, const Pair& dest, const float grid_resolution_m, const float padding_dist_m)
{
    PADDING_DIST_M = padding_dist_m;
	const float grid_resolution_cm = grid_resolution_m * 100;

	Status status = Status::A_STAR_SUCCESS;
	int src_clearance = SOURCE_OBSTACLE_CLEARANCE_RADIUS_M / grid_resolution_m;
	int dst_clearance = DEST_OBSTACLE_CLEARANCE_RADIUS_M / grid_resolution_m;

	// If the destination is in an obstacle
	if (!isSafe(grid, dest)) {
		status |= Status::A_STAR_DEST_OBSTACLE;
		clear_obstacles_from_location(grid, dest, dst_clearance);
	}

	// put a border around the outside of the map
	add_obstacle_border(grid);

	// assign heuristic values according to distance to nearest obstacle
	precompute_padding_values(grid, grid_resolution_cm);

	// If the destination cell is the same as source cell, we have already found it
	if (isDestination(src, dest)) {
		return construct_return_val(vector<Pair> {{src}}, status);
	}

	// If the source is in an obstacle
	if (!isSafe(grid, src)) {
		status |= Status::A_STAR_START_OBSTACLE;
		clear_obstacles_from_location(grid, src, src_clearance);
	}


	// Create a closed list and initialise it to false which
	// means that no cell has been included yet This closed
	// list is implemented as a boolean 2D array
	bool closedList[COL][ROW];
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

	// for storing the nearest point to the destination we could find
	double min_dist_to_dest_squared = C_INF;
	Pair nearest_point = src;

	// We set this boolean value as false as initially
	// the destination is not reached.
	while (!openList.empty()) {
		const Tuple& p = openList.top();
		// Add this vertex to the closed list
		i = get<1>(p); // second element of tuple
		j = get<2>(p); // third element of tuple

		// Remove this vertex from the open list
		openList.pop();
		closedList[i][j] = true;

		// Generating all the 8 successors of this cell
	    for (Pair branch : BRANCHES) {
			int diff_x = branch.first, diff_y = branch.second;

			Pair neighbour(i + diff_x, j + diff_y);
			
			// If the cell doesn't contain an obstacle, is in the map, and hasn't already been explored
			if (isSafe(grid, neighbour) 
				&& isValid(COL, ROW, neighbour) 
				&& !closedList[neighbour.first][neighbour.second]) {
				
				if (isDestination(neighbour, dest)) {
					// We have found the destination!
					// Set the parent to the current cell
					cellDetails[neighbour.first][neighbour.second].parent = { i, j };
					vector<Pair> path = tracePath(cellDetails, dest);

					return construct_return_val(path, status);
				}
				
				float gNew, hNew, fNew;
				// additional distance to next point
				float g_diff = (diff_y == 0 || diff_x == 0) ? 1.0 : sqrt(2.0);
				gNew = cellDetails[i][j].g + g_diff;
				// Heuristic contains weighted combination of the terrain cost of driving here and the distance to the goal
				hNew = grid[neighbour.first][neighbour.second] * TERRAIN_IMPORTANCE + heuristic(neighbour, dest) * WEIGHT;
				fNew = gNew + hNew;

				// In case we can't get to the destination, we keep track of the best effort so far:
				// A point that optimises between being as close to the goal as possible while not requiring us to drive too far
				double dist_sqrd = dist_squared(neighbour, dest);
				if (NEAREST_POINT_DIST_WEIGHT * dist_sqrd + gNew < min_dist_to_dest_squared) {
					// if the distance is much better, 
					min_dist_to_dest_squared = NEAREST_POINT_DIST_WEIGHT * dist_sqrd + gNew; 
					nearest_point = neighbour;
				}
				
				// If we hadn't explored this cell before, or we have found a new, shorter path to it
				if (cellDetails[neighbour.first][neighbour.second].g == -1
					|| cellDetails[neighbour.first][neighbour.second].g > gNew) {
					// Put this cell in the open list
					openList.emplace(fNew, neighbour.first, neighbour.second);

					// Update the details of this cell
					cellDetails[neighbour.first][neighbour.second].g = gNew;
					cellDetails[neighbour.first][neighbour.second].parent = { i, j };
				}
			}
		}
	}

	// When the destination cell is not found and the open
	// list is empty, then we conclude that we failed to
	// reach the destiantion cell. This may happen when the
	// there is no way to destination cell (due to
	// blockages)
	status |= Status::A_STAR_NO_PATH;
	
	// instead return path to the nearest point we could find
	vector<Pair> path = tracePath(cellDetails, nearest_point);

	if (path.size() < CRITICAL_PATH_LEN) status |= Status::A_STAR_CRITICAL_NO_PATH;
	return construct_return_val(path, status);
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
    vector<Pair> emergency_path = aStarSearch(grid, src, dest, 2.5, 1);
	return 0;
}

PYBIND11_MODULE(a_star, module_handle) {
    module_handle.doc() = "Nova Rover A* C++ search algorithm binded to Python3";
    module_handle.def("a_star", &aStarSearch<200, 200>); // 10 cm resolution
}
