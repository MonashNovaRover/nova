"""
Purpose of this is to abstract away some of the librealsense components related to an image, its key-points, depth etc.
"""
import time

import D415
import numpy as np
import pyrealsense2 as rs
import cv2
import matplotlib
import matplotlib.pyplot as plt

"""
Class used to hold various aspects of a (stereo) image, including its gray scale, color, depth versions, and features
"""

max_error = 10

max_corners = 100
quality_level = 0.01
min_distance = 10
block_size = 3


class RSImage:
    def __init__(self, frames, camera: D415):
        depth_frame = frames.get_depth_frame()
        color_frame = frames.get_color_frame()

        depth_frame = decimate.process(depth_frame)

        # Grab new intrinsics (may be changed by decimation)
        depth_intrinsics = rs.video_stream_profile(
            depth_frame.profile).get_intrinsics()
        w, h = depth_intrinsics.width, depth_intrinsics.height

        depth_image = np.asanyarray(depth_frame.get_data())
        color_image = np.asanyarray(color_frame.get_data())

        depth_colormap = np.asanyarray(
            colorizer.colorize(depth_frame).get_data())

        if state.color:
            mapped_frame, color_source = color_frame, color_image
        else:
            mapped_frame, color_source = depth_frame, depth_colormap

        points = pc.calculate(depth_frame)
        pc.map_to(mapped_frame)

        # Pointcloud data to arrays
        v, t = points.get_vertices(), points.get_texture_coordinates()
        self.verts = np.asanyarray(v).view(np.float32).reshape(-1, 3)  # xyz
        texcoords = np.asanyarray(t).view(np.float32).reshape(-1, 2)  # uv

    def get_euclidean_cords(self, img_row, img_col):
        depth = self.aligned_depth_frame.get_distance(img_row, img_col)
        # NOTE: do I need to flip the bellow rows and column ordering?!?
        return rs.rs2_deproject_pixel_to_point(self.depth_intrinsics, [img_col, img_row], depth)

    def get_image_features(self):
        # never re-compute
        if self.features:
            return self.features

        grey_image = cv2.cvtColor(self.color_image, cv2.COLOR_BGR2GRAY)
        feature_points = cv2.goodFeaturesToTrack(grey_image, maxCorners=max_corners,
                                                 qualityLevel=quality_level,
                                                 minDistance=min_distance, blockSize=block_size)

        feature_points = np.int0(feature_points)
        self.features = feature_points
        return feature_points

    def calculate_sequential_features(self, previous_image, display=False):
        # calculate features based on previous features and previous and current image
        grey_previous = cv2.cvtColor(previous_image.color_image, cv2.COLOR_BGR2GRAY)
        grey_next = cv2.cvtColor(self.color_image, cv2.COLOR_BGR2GRAY)
        prev_points = previous_image.get_image_features()
        new_points, status, error = cv2.calcOpticalFlowPyrLK(prevImg=grey_previous, nextImg=grey_next,
                                                             prevPts=np.float32(prev_points),
                                                             nextPts=None)

        # filter out and re-format features which don't meet status and have x error
        prev_points = [list(prev_points[i][0]) for i in range(0, len(status)) if status[i] and error[i] <= max_error]
        new_points = [list(new_points[i][0]) for i in range(0, len(status)) if status[i] and error[i] <= max_error]
        assert len(prev_points) == len(new_points)

        # now, we need to look at which features will
        good_old = []
        good_new = []

        old = np.int0(prev_points)
        new = np.int0(new_points)

        for i in range(0, len(new)):
            try:
                size = old[i][0] < 640 and old[i][1] < 480 and new[i][0] < 640 and new[i][1] < 480
                if size:
                    depth = self.aligned_depth_frame.get_distance(old[i][0], old[i][
                        1]) > 0 and self.aligned_depth_frame.get_distance(new[i][0], new[i][1]) > 0
                    if depth:
                        good_old.append(old[i])
                        good_new.append(new[i])
            except Exception:
                print("some error in testing depths")

        if display:
            for i in good_old:
                x, y = i.ravel()
                cv2.circle(self.color_image_draw, (x, y), 3, 255, -1)
            for i in good_new:
                x, y = i.ravel()
                cv2.circle(self.color_image_draw, (x, y), 3, 150, -1)

            plt.imshow(self.color_image_draw, aspect="auto")
            plt.show()
            plt.pause(0.05)

        good_new = np.int0(good_new)
        good_old = np.int0(good_old)
        return good_old, good_new


def old_example():
    # the following shows how this class is meant to be used
    _camera = D415.D415()
    image0 = RSImage(_camera.pipeline.wait_for_frames(), _camera)
    time.sleep(0.2)
    image1 = RSImage(_camera.pipeline.wait_for_frames(), _camera)
    time.sleep(0.5)
    image2 = RSImage(_camera.pipeline.wait_for_frames(), _camera)
    print(image2.calculate_sequential_features(image1, display=True))
    input("Press any key to exit")


def get_point_clouds():
    _camera = D415.D415()
    image0 = RSImage(_camera.pipeline.wait_for_frames(), _camera)
    print(image0.verts)


if __name__ == "__main__":
    get_point_clouds()




