__package__ = "autonomous"
from planning.path_planner import PathPlanner

from config.runtime_params import *
import matplotlib.pyplot as plt
import rclpy

class TestMapMaker:
    def __init__(self, sizex, sizey):
        self.map = [[0 for _ in range(sizex)] for _ in range(sizey)]
        self.x = sizex
        self.y = sizey

    def generate_map(self):
        pass

class TestBoxMap(TestMapMaker):
    def __init__(self, sizex, sizey):
        super().__init__(sizex, sizey)

    def generate_obstacles(self):
        for i in range(30):
            self.map[15][20 + i] = 1

        for i in range(20):
            self.map[15 + i][20] = 1

        for i in range(30):
            self.map[35][20 + i] = 1
        
        for i in range(20):
            self.map[15 + i][50] = 1

        return self.map

class TestLinesMap(TestMapMaker):
    def __init__(self, sizex, sizey):
        super().__init__(sizex, sizey)
  
    def generate_obstacles(self):
        for i in range(25):
            self.map[48][0 + i] = 1

        for i in range(15):
            self.map[42 + i][38] = 1

        for i in range(30):
            self.map[33][23 + i] = 1
        
        for i in range(20):
            self.map[15 + i][53] = 1

        return self.map

if __name__ == "__main__":
    rclpy.init(args = None)

    map_maker = TestBoxMap(70, 70)
    
    this_map = map_maker.generate_obstacles()

    map2d = Map2D(length=3.5, width=3.5, resolution=0.05)

    map2d.grid = np.array(this_map)

    planner = PathPlanner([0.5, 0.0], map2d)

    for startx in [-1.5]:#, -1., -0.5, 0., 0.5, 1.0, 1.5]:
        for starty in [-1.5, -1, -0.5, 0, 0.5, 1.0, 1.5]:
            this_map[int(startx * 20 + 35)][int(starty * 20 + 35)] = 2

            planner.start = (startx, starty)

            A_star_path = planner.aStar(planner.start, planner.goal)
            path = planner.stringPull(A_star_path)
            padded_path = planner.pad_corners(path)
            clear_path = planner.clear_path_to_first_waypoint(padded_path, 1.0, 1)

            route_coordinates = planner.get_local_coords_route(path)

            path = np.array(path)
            padded_path = np.array(padded_path)
            A_star_path = np.array(A_star_path)
            clear_path = np.array(clear_path)

            plt.figure()
            
            try:
                plt.plot(A_star_path[:,1], A_star_path[:,0], label="A* path")
                plt.plot(padded_path[:,1], padded_path[:,0], label="Circle padding")
                plt.plot(clear_path[:,1], clear_path[:,0], label="Circles + obstacle detection")
                plt.plot(path[:,1], path[:,0], 'o')
                
            except Exception as e:
                print(e)
                print("path = " + str(path))
                print("padded_path = " + str(padded_path))
                continue
            
            plt.xlim = [-35, 35]
            plt.ylim = [-35, 35]

            plt.legend()

            plt.imshow(this_map)

            plt.savefig("x=%.1f_y=%.1f.png" % (startx, starty))

            this_map[int(startx * 20 + 35)][int(starty * 20 + 35)] = 0
    rclpy.shutdown()

