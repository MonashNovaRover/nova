import numpy as np
import open3d as o3d


xyz_load = np.load('basicPCL.npy')
print('xyz_load')
print(xyz_load)
print(xyz_load.shape, max(xyz_load[0]))
#img = o3d.geometry.Image(.astype(np.uint8))
pcd = o3d.geometry.PointCloud()
pcd.points = o3d.utility.Vector3dVector(xyz_load)
o3d.visualization.draw_geometries([pcd])

