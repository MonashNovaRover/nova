# RTABMap Parameters Summary

Refer to this [Parameters.h](https://github.com/introlab/rtabmap/blob/master/corelib/include/rtabmap/core/Parameters.h) for more information about these parameters.
Also probs look at `rtabmap_defaults.yaml` :)

## General Parameters
| Parameter                           | Type           | Default Value | Description                                                                                                                                               |
|-------------------------------------|----------------|---------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------|
| Rtabmap.PublishStats                | bool           | true          | Publishing statistics.                                                                                                                                   |
| Rtabmap.PublishLastSignature        | bool           | true          | Publishing last signature.                                                                                                                                 |
| Rtabmap.PublishPdf                  | bool           | true          | Publishing pdf.                                                                                                                                           |
| Rtabmap.PublishLikelihood           | bool           | true          | Publishing likelihood.                                                                                                                                     |
| Rtabmap.PublishRAMUsage             | bool           | false         | Publishing RAM usage in statistics (may add a small overhead to get info from the system).                                                                |
| Rtabmap.ComputeRMSE                 | bool           | true          | Compute root mean square error (RMSE) and publish it in statistics, if ground truth is provided.                                                        |
| Rtabmap.SaveWMState                 | bool           | false         | Save working memory state after each update in statistics.                                                                                                |
| Rtabmap.TimeThr                     | float          | 0             | Maximum time allowed for map update (ms) (0 means infinity). When map update time exceeds this fixed time threshold, some nodes are transferred to LTM.   |
| Rtabmap.MemoryThr                   | int            | 0             | Maximum nodes in the Working Memory (0 means infinity). When the number of nodes in Working Memory exceeds this threshold, some nodes are transferred to LTM. |
| Rtabmap.DetectionRate               | float          | 1             | Detection rate (Hz). RTAB-Map will filter input images to satisfy this rate.                                                                             |
| Rtabmap.ImageBufferSize             | unsigned int   | 1             | Data buffer size (0 means infinite).                                                                                                                      |
| Rtabmap.CreateIntermediateNodes     | bool           | false         | Create intermediate nodes between loop closure detection. Only used when DetectionRate > 0.                                                              |
| Rtabmap.WorkingDirectory            | string         | ""            | Working directory.                                                                                                                                       |
| Rtabmap.MaxRetrieved                | unsigned int   | 2             | Maximum nodes retrieved at the same time from LTM.                                                                                                       |
| Rtabmap.MaxRepublished              | unsigned int   | 2             | Maximum nodes republished when requesting missing data. Ignored if PublishLastSignature=false.                                                           |
| Rtabmap.StatisticLogsBufferedInRAM  | bool           | true          | Statistic logs buffered in RAM instead of written to hard drive after each iteration.                                                                  |
| Rtabmap.StatisticLogged             | bool           | false         | Logging enabled.                                                                                                                                         |
| Rtabmap.StatisticLoggedHeaders      | bool           | true          | Add column header description to log files.                                                                                                              |
| Rtabmap.StartNewMapOnLoopClosure    | bool           | false         | Start a new map only if there is a global loop closure with a previous map.                                                                              |
| Rtabmap.StartNewMapOnGoodSignature  | bool           | false         | Start a new map only if the first signature is not bad (i.e., has enough features).                                                                     |
| Rtabmap.ImagesAlreadyRectified      | bool           | true          | Images are already rectified. By default RTAB-Map assumes that received images are rectified.                                                            |
| Rtabmap.RectifyOnlyFeatures         | bool           | false         | If "ImagesAlreadyRectified" is false and this parameter is true, only the features are rectified, not the whole image. Warning: The point cloud map colors may be wrong. |


## Grid Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `Grid.Sensor` | `int` | `1` | Create occupancy grid from selected sensor: `0=laser scan`, `1=depth image(s)`, or `2=both laser scan and depth image(s)`. |
| `Grid.DepthDecimation` | `unsigned int` | `4` | `[Grid.Sensor=true]` Decimation of the depth image before creating cloud. |
| `Grid.RangeMin` | `float` | `0.0` | Minimum range from sensor. |
| `Grid.RangeMax` | `float` | `5.0` | Maximum range from sensor. `0=inf`. |
| `Grid.DepthRoiRatios` | `string` | `"0.0 0.0 0.0 0.0"` | `[Grid.Sensor>=1]` Region of interest ratios `[left, right, top, bottom]`. |
| `Grid.FootprintLength` | `float` | `0.0` | Footprint length used to filter points over the footprint of the robot. |
| `Grid.FootprintWidth` | `float` | `0.0` | Footprint width used to filter points over the footprint of the robot. Footprint length should be set. |
| `Grid.FootprintHeight` | `float` | `0.0` | Footprint height used to filter points over the footprint of the robot. Footprint length and width should be set. |
| `Grid.ScanDecimation` | `int` | `1` | `[Grid.Sensor=0 or 2]` Decimation of the laser scan before creating cloud. |
| `Grid.CellSize` | `float` | `0.05` | Resolution of the occupancy grid. |
| `Grid.PreVoxelFiltering` | `bool` | `true` | Input cloud is downsampled by voxel filter before segmentation of obstacles and ground. |
| `Grid.MapFrameProjection` | `bool` | `false` | Projection in map frame. Disabling projects in robot frame instead. |
| `Grid.NormalsSegmentation` | `bool` | `true` | Segment ground from obstacles using point normals. |
| `Grid.MaxObstacleHeight` | `float` | `0.0` | Maximum obstacles height (`0=disabled`). |
| `Grid.MinGroundHeight` | `float` | `0.0` | Minimum ground height (`0=disabled`). |
| `Grid.MaxGroundHeight` | `float` | `0.0` | `[Grid.NormalsSegmentation=false]` Maximum ground height (`0=disabled`). |
| `Grid.MaxGroundAngle` | `float` | `45` | `[Grid.NormalsSegmentation=true]` Maximum angle (degrees) between point's normal and ground's normal to label it as ground. |
| `Grid.NormalK` | `int` | `20` | `[Grid.NormalsSegmentation=true]` K neighbors to compute normals. |
| `Grid.ClusterRadius` | `float` | `0.1` | `[Grid.NormalsSegmentation=true]` Cluster maximum radius. |
| `Grid.MinClusterSize` | `int` | `10` | `[Grid.NormalsSegmentation=true]` Minimum cluster size to project the points. |
| `Grid.FlatObstacleDetected` | `bool` | `true` | `[Grid.NormalsSegmentation=true]` Flat obstacles detected. |
| `Grid.3D` | `bool` | `true` (`false` if OctoMap is not enabled) | A 3D occupancy grid is required for OctoMap (3D ray tracing). |
| `Grid.GroundIsObstacle` | `bool` | `false` | `[Grid.3D=true]` Ground segmentation is ignored, all points are obstacles. Useful for UAVs. |
| `Grid.NoiseFilteringRadius` | `float` | `0.0` | Noise filtering radius (`0=disabled`). |
| `Grid.NoiseFilteringMinNeighbors` | `int` | `5` | Noise filtering minimum neighbors. |
| `Grid.Scan2dUnknownSpaceFilled` | `bool` | `false` | Unknown space filled. Only used with 2D laser scans. |
| `Grid.RayTracing` | `bool` | `false` | Ray tracing for occupied cells, filling unknown space between the sensor and occupied cells. |

## Global Grid Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `GridGlobal.UpdateError` | `float` | `0.01` | Graph changed detection error (m). |
| `GridGlobal.FootprintRadius` | `float` | `0.0` | Footprint radius (m) used to clear all obstacles under the graph. |
| `GridGlobal.MinSize` | `float` | `0.0` | Minimum map size (m). |
| `GridGlobal.Eroded` | `bool` | `false` | Erode obstacle cells. |
| `GridGlobal.MaxNodes` | `int` | `0` | Maximum nodes assembled in the map (0=unlimited). |
| `GridGlobal.AltitudeDelta` | `float` | `0.0` | Assemble only nodes with altitude within `±delta` meters of the current pose (`0=disabled`). |
| `GridGlobal.OccupancyThr` | `float` | `0.5` | Occupancy threshold (0-1). |
| `GridGlobal.ProbHit` | `float` | `0.7` | Probability of a hit (`0.5-1`). |
| `GridGlobal.ProbMiss` | `float` | `0.4` | Probability of a miss (`0-0.5`). |
| `GridGlobal.ProbClampingMin` | `float` | `0.1192` | Probability clamping minimum (`0-1`). |
| `GridGlobal.ProbClampingMax` | `float` | `0.971` | Probability clamping maximum (`0-1`). |
| `GridGlobal.FloodFillDepth` | `unsigned int` | `0` | Flood fill filter (`0=disabled`), removes empty cells outside the map. |

## RGBD SLAM Parameters

| Parameter                              | Type         | Default Value | Description                                                                                                                           |
|----------------------------------------|--------------|---------------|---------------------------------------------------------------------------------------------------------------------------------------|
| `RGBD.Enabled`                         | bool         | true          | Activate metric SLAM. If set to false, classic RTAB-Map loop closure detection is done using only images and without any metric info.  |
| `RGBD.LinearUpdate`                    | float        | 0.1           | Minimum linear displacement (m) to update the map. Rehearsal is done prior to this, so weights are still updated.                      |
| `RGBD.AngularUpdate`                   | float        | 0.1           | Minimum angular displacement (rad) to update the map. Rehearsal is done prior to this, so weights are still updated.                   |
| `RGBD.LinearSpeedUpdate`               | float        | 0.0           | Maximum linear speed (m/s) to update the map (0 means not limit).                                                                     |
| `RGBD.AngularSpeedUpdate`              | float        | 0.0           | Maximum angular speed (rad/s) to update the map (0 means not limit).                                                                  |
| `RGBD.AggressiveLoopThr`               | float        | 0.05          | Loop closure threshold used when a new mapping session is not yet linked to a map of the highest loop closure hypothesis.              |
| `RGBD.NewMapOdomChangeDistance`        | float        | 0             | A new map is created if a change of odometry translation greater than X m is detected (0 m = disabled).                               |
| `RGBD.OptimizeFromGraphEnd`            | bool         | false         | Optimize graph from the newest node. If false, the graph is optimized from the oldest node of the current graph.                      |
| `RGBD.OptimizeMaxError`                | float        | 3.0           | Reject loop closures if optimization error ratio is greater than this value (0=disabled).                                             |
| `RGBD.MaxLoopClosureDistance`          | float        | 0.0           | Reject loop closures/localizations if the distance from the map is over this distance (0=disabled).                                    |
| `RGBD.ForceOdom3DoF`                   | bool         | true          | Force odometry pose to be 3DoF if true.                                                                                                |
| `RGBD.StartAtOrigin`                   | bool         | false         | If true, rtabmap will assume the robot is starting from the origin of the map.                                                        |
| `RGBD.GoalReachedRadius`               | float        | 0.5           | Goal reached radius (m).                                                                                                               |
| `RGBD.PlanStuckIterations`             | int          | 0             | Mark the current goal node on the path as unreachable if it is not updated after X iterations (0=disabled).                          |
| `RGBD.PlanLinearVelocity`              | float        | 0             | Linear velocity (m/sec) used to compute path weights.                                                                                  |
| `RGBD.PlanAngularVelocity`             | float        | 0             | Angular velocity (rad/sec) used to compute path weights.                                                                               |
| `RGBD.GoalsSavedInUserData`            | bool         | false         | When a goal is received and processed with success, it is saved in user data of the location.                                           |
| `RGBD.MaxLocalRetrieved`               | unsigned int | 2             | Maximum local locations retrieved (0=disabled) near the current pose in the local map or on the current planned path.                 |
| `RGBD.LocalRadius`                     | float        | 10            | Local radius (m) for nodes selection in the local map.                                                                                 |
| `RGBD.LocalImmunizationRatio`          | float        | 0.25          | Ratio of working memory for which local nodes are immunized from transfer.                                                             |
| `RGBD.ScanMatchingIdsSavedInLinks`     | bool         | true          | Save scan matching IDs from one-to-many proximity detection in link's user data.                                                      |
| `RGBD.NeighborLinkRefining`            | bool         | false         | Refine the transformation of neighbor links using registration approach selected.                                                     |
| `RGBD.LoopClosureIdentityGuess`        | bool         | false         | Use Identity matrix as guess when computing loop closure transform.                                                                    |
| `RGBD.LoopClosureReextractFeatures`    | bool         | false         | Extract features even if there are some already in the nodes. Raw features are not saved in database.                                  |
| `RGBD.LocalBundleOnLoopClosure`        | bool         | false         | Do local bundle adjustment with neighborhood of the loop closure.                                                                       |
| `RGBD.InvertedReg`                     | bool         | false         | On loop closure, do registration from the target to reference instead of reference to target.                                          |
| `RGBD.CreateOccupancyGrid`             | bool         | false         | Create local occupancy grid maps.                                                                                                      |
| `RGBD.MarkerDetection`                 | bool         | false         | Detect static markers to be added as landmarks for graph optimization.                                                                  |
| `RGBD.LoopCovLimited`                  | bool         | false         | Limit covariance of non-neighbor links to minimum covariance of neighbor links.                                                        |
| `RGBD.MaxOdomCacheSize`                | int          | 10            | Maximum odometry cache size. Used only in localization mode. Set 0 to disable caching.                                                |
| `RGBD.LocalizationSmoothing`           | bool         | true          | Adjust localization constraints based on optimized odometry cache poses.                                                                |
| `RGBD.LocalizationPriorError`          | double       | 0.001         | The corresponding variance (error x error) set to priors of the map's poses during localization.                                       |
| `RGBD.ProximityByTime`                 | bool         | false         | Detection over all locations in STM.                                                                                                  |
| `RGBD.ProximityBySpace`                | bool         | true          | Detection over locations (in Working Memory) near in space.                                                                            |
| `RGBD.ProximityMaxGraphDepth`          | int          | 50            | Maximum depth from the current/last loop closure location and the local loop closure hypotheses.                                       |
| `RGBD.ProximityMaxPaths`               | int          | 3             | Maximum paths compared for proximity detection.                                                                                         |
| `RGBD.ProximityPathFilteringRadius`    | float        | 1             | Path filtering radius to reduce the number of nodes to compare in a path in one-to-many proximity detection.                           |
| `RGBD.ProximityPathMaxNeighbors`       | int          | 0             | Maximum neighbor nodes compared on each path for proximity detection.                                                                  |
| `RGBD.ProximityPathRawPosesUsed`       | bool         | true          | Use the raw odometry poses instead of optimized ones for proximity detection.                                                          |
| `RGBD.ProximityAngle`                  | float        | 45            | Maximum angle (degrees) for one-to-one proximity detection.                                                                             |
| `RGBD.ProximityOdomGuess`              | bool         | false         | Use odometry as motion guess for one-to-one proximity detection.                                                                       |
| `RGBD.ProximityGlobalScanMap`          | bool         | false         | Create a global assembled map from laser scans for one-to-many proximity detection.                                                    |
| `RGBD.ProximityMergedScanCovFactor`    | double       | 100.0         | Covariance factor for one-to-many proximity detection when scans are used.                                                              |


## VO Parameters

| Parameter                              | Type            | Default | Description |
|---------------------------------------------|-----------------|---------------|-------------|
| `Vis.EstimationType`                        | int             | 1             | Motion estimation approach: 0=3D->3D, 1=3D->2D (PnP), 2=2D->2D (Epipolar Geometry) |
| `Vis.ForwardEstOnly`                        | bool            | true          | Forward estimation only (A->B). If false, a transformation is also computed in backward direction (B->A), then the two resulting transforms are merged (middle interpolation between the transforms). |
| `Vis.InlierDistance`                        | float           | 0.1           | Maximum distance for feature correspondences. Used by 3D->3D estimation approach. |
| `Vis.RefineIterations`                      | int             | 5             | Number of iterations used to refine the transformation found by RANSAC. 0 means that the transformation is not refined. |
| `Vis.PnPReprojError`                        | float           | 2             | PnP reprojection error. |
| `Vis.PnPFlags`                              | int             | 0             | PnP flags: 0=Iterative, 1=EPNP, 2=P3P. |
| `Vis.PnPRefineIterations`                   | int             | 0 or 1        | Refine iterations. Set to 0 if Bundle Adjustment is also used. |
| `Vis.PnPVarianceMedianRatio`                | int             | 4             | Ratio used to compute variance of the estimated transformation if 3D correspondences are provided. |
| `Vis.PnPMaxVariance`                        | float           | 0.0           | Max linear variance between 3D point correspondences after PnP. 0 means disabled. |
| `Vis.PnPSamplingPolicy`                     | unsigned int    | 1             | Multi-camera random sampling policy: 0=AUTO, 1=ANY, 2=HOMOGENEOUS. |
| `Vis.PnPSplitLinearCovComponents`           | bool            | false         | Compute variance for each linear component instead of using the combined XYZ variance for all linear components. |
| `Vis.EpipolarGeometryVar`                   | float           | 0.1           | Epipolar geometry maximum variance to accept the transformation. |
| `Vis.MinInliers`                            | int             | 20            | Minimum feature correspondences to compute/accept the transformation. |
| `Vis.MeanInliersDistance`                   | float           | 0.0           | Maximum distance of the mean distance of inliers from the camera to accept the transformation. 0 means disabled. |
| `Vis.MinInliersDistribution`                | float           | 0.0           | Minimum distribution value of the inliers in the image to accept the transformation. |
| `Vis.Iterations`                            | int             | 300           | Maximum iterations to compute the transform. |
| `Vis.FeatureType`                           | int             | 6 or 8        | Feature type: Various options such as SURF, SIFT, ORB, etc. |
| `Vis.MaxFeatures`                           | int             | 1000          | Maximum number of features. 0 means no limits. |
| `Vis.SSC`                                   | bool            | false         | If true, SSC (Suppression via Square Covering) is applied to limit keypoints. |
| `Vis.MaxDepth`                              | float           | 0.0           | Max depth of the features. 0 means no limit. |
| `Vis.MinDepth`                              | float           | 0.0           | Min depth of the features. 0 means no limit. |
| `Vis.DepthAsMask`                           | bool            | true          | Use depth image as mask when extracting features. |
| `Vis.DepthMaskFloorThr`                     | float           | 0.0           | Filter floor from depth mask below specified threshold before extracting features. |
| `Vis.RoiRatios`                             | string          | "0.0 0.0 0.0 0.0" | Region of interest ratios [left, right, top, bottom]. |
| `Vis.SubPixWinSize`                         | int             | 3             | See cv::cornerSubPix(). |
| `Vis.SubPixIterations`                      | int             | 0             | See cv::cornerSubPix(). 0 disables sub pixel refining. |
| `Vis.SubPixEps`                             | float           | 0.02          | See cv::cornerSubPix(). |
| `Vis.GridRows`                              | int             | 1             | Number of rows of the grid used to extract uniformly features from each cell. |
| `Vis.GridCols`                              | int             | 1             | Number of columns of the grid used to extract uniformly features from each cell. |
| `Vis.CorType`                               | int             | 0             | Correspondences computation approach: 0=Features Matching, 1=Optical Flow. |
| `Vis.CorNNType`                             | int             | 1             | kNNFlannNaive=0, kNNFlannKdTree=1, etc. Used for feature matching. |
| `Vis.CorNNDR`                               | float           | 0.8           | Nearest neighbor distance ratio used for knn feature matching approach. |
| `Vis.CorGuessWinSize`                       | int             | 40            | Matching window size (pixels) around projected points when a guess transform is provided. |
| `Vis.CorGuessMatchToProjection`             | bool            | false         | Match frame's corners to source's projected points when a guess transform is provided. |
| `Vis.CorFlowWinSize`                        | int             | 16            | See cv::calcOpticalFlowPyrLK(). Used for optical flow approach. |
| `Vis.CorFlowIterations`                     | int             | 30            | See cv::calcOpticalFlowPyrLK(). Used for optical flow approach. |
| `Vis.CorFlowEps`                            | float           | 0.01          | See cv::calcOpticalFlowPyrLK(). Used for optical flow approach. |
| `Vis.CorFlowMaxLevel`                       | int             | 3             | See cv::calcOpticalFlowPyrLK(). Used for optical flow approach. |
| `Vis.CorFlowGpu`                            | bool            | false         | Enable GPU version of the optical flow approach (only available if OpenCV is built with CUDA). |
