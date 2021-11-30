
# extra notes
# downsampling test to pointcloud
#print("Downsample the point cloud with a voxel of 0.05")
#downpcd = pointSet.voxel_down_sample(voxel_size=0.1) #voxel
#o3d.visualization.draw_geometries([downpcd], zoom=0.3412, front=[0.4257, -0.2125, -0.8795], lookat=[2.6172, 2.0475, 1.532], up=[-0.0694, -0.9768, 0.2024])

# seems to loop forever using this method so it never continues to the next point cloud. Thus non-blocking visulaization!
#o3d.visualization.draw_geometries([pointSet],
                       #   zoom=0.3412,
                       #   front=[0.4257, -0.2125, -0.8795],
                       #   lookat=[2.6172, 2.0475, 1.532],
                       #   up=[-0.0694, -0.9768, 0.2024])

# example code given for non-blocking visualization
def testfunc(self, pc1, pc2):
    o3d.utility.set_verbosity_level(o3d.utility.VerbosityLevel.Debug)
    source = self.pc1.voxel_down_sample(voxel_size=0.02)
    target = self.pc2.voxel_down_sample(voxel_size=0.02)
    trans = [[0.862, 0.011, -0.507, 0.0], [-0.139, 0.967, -0.215, 0.7],
             [0.487, 0.255, 0.835, -1.4], [0.0, 0.0, 0.0, 1.0]]
    source.transform(trans)

    flip_transform = [[1, 0, 0, 0], [0, -1, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1]]
    source.transform(flip_transform)
    target.transform(flip_transform)

    vis = o3d.visualization.Visualizer()
    vis.create_window()
    vis.add_geometry(source)
    vis.add_geometry(target)
    threshold = 0.05
    icp_iteration = 100
    save_image = False

    for i in range(icp_iteration):
        reg_p2l = o3d.pipelines.registration.registration_icp(
            source, target, threshold, np.identity(4),
            o3d.pipelines.registration.TransformationEstimationPointToPlane(),
            o3d.pipelines.registration.ICPConvergenceCriteria(max_iteration=1))
        source.transform(reg_p2l.transformation)
        vis.update_geometry(source)
        vis.poll_events()
        vis.update_renderer()
        if save_image:
            vis.capture_screen_image("temp_%04d.jpg" % i)
    vis.destroy_window()
