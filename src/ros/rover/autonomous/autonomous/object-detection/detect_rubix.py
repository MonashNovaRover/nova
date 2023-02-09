import cv2
import time
import numpy as np
import torch
import pyrealsense2 as rs
def get_depth_avg(n, depth_frame, cx, cy, point):
    avg_depth = [0,0,0] if point else 0
    total_pixels = 0
    for i in range(-n//2, n//2):
        for j in range(-n//2, n//2):
            if point:
                curr_point = rs.rs2_deproject_pixel_to_point(intr, [float(cx -i), float(cy-j)],
                                                             depth_frame.get_distance(cx-i, cy-j))
                for k in range(3):
                    avg_depth[k] += curr_point[k]
            else:
                avg_depth += depth_frame.get_distance(cx-i, cy-j)
            total_pixels += 1
    return avg_depth/total_pixels

INPUT_WIDTH = 640
INPUT_HEIGHT = 640
CONFIDENCE_THRESHOLD = 0.6

model = torch.hub.load('/home/ecthelion/yolov5', 'custom', path='/home/ecthelion/yolov5/rubix.pt', source='local')

start = time.time_ns()
frame_count = 0
total_frames = 0
fps = -1
class_list = ['rubix-cube']

#capture = cv2.VideoCapture(0)

# Configure depth and color streams
pipeline = rs.pipeline()
config = rs.config()

# Get device product line for setting a supporting resolution
pipeline_wrapper = rs.pipeline_wrapper(pipeline)
pipeline_profile = config.resolve(pipeline_wrapper)
device = pipeline_profile.get_device()
device_product_line = str(device.get_info(rs.camera_info.product_line))

found_rgb = False
for s in device.sensors:
    if s.get_info(rs.camera_info.name) == 'RGB Camera':
        found_rgb = True
        break
if not found_rgb:
    print("The demo requires Depth camera with Color sensor")
    exit(0)

config.enable_stream(rs.stream.depth, 640, 480, rs.format.z16, 30)

if device_product_line == 'L500':
    config.enable_stream(rs.stream.color, 960, 540, rs.format.bgr8, 30)
else:
    config.enable_stream(rs.stream.color, 640, 480, rs.format.bgr8, 30)

# Start streaming
cfg = pipeline.start(config)
intr = cfg.get_stream(rs.stream.depth).as_video_stream_profile().get_intrinsics()
print(type(intr))
while True:
    frames = pipeline.wait_for_frames()
    depth_frame = frames.get_depth_frame()
    color_frame = frames.get_color_frame()
    color_image = np.asanyarray(color_frame.get_data())

    results = model(color_image).xyxy[0]

    frame_count += 1
    total_frames += 1

    for xmin, ymin, xmax, ymax, confidence, cl in results:
        if confidence > CONFIDENCE_THRESHOLD:
            cx, cy = int(xmin) + (int(xmax) - int(xmin))//2, int(ymin) + (int(ymax) - int(ymin))//2
            depth = depth_frame.get_distance(cx,cy)
            point = rs.rs2_deproject_pixel_to_point(intr, [float(cx), float(cy)], depth)
            print(f'cx: {cx}, cy: {cy}')
            print(f'Depth: {depth:.2f}')
            print(f'Point: {point[0]}')
            cv2.rectangle(color_image, (int(xmin), int(ymin)), (int(xmax), int(ymax)), (0,0,0), 2)
            cv2.putText(color_image, class_list[int(cl)], (int(xmin), int(ymin) - 30), cv2.FONT_HERSHEY_SIMPLEX, .5, (0, 0, 255))
            cv2.putText(color_image, f'Depth: {depth:.2f}', (int(xmin), int(ymin) - 20), cv2.FONT_HERSHEY_SIMPLEX, .5, (0, 0, 255))
            #cv2.putText(color_image, f'coords: {point}', (int(xmin), int(ymin) - 10), cv2.FONT_HERSHEY_SIMPLEX, .5,
                        #(0, 0, 255))

    if frame_count >= 30:
        end = time.time_ns()
        fps = 1000000000 * frame_count / (end - start)
        frame_count = 0
        start = time.time_ns()

    if fps > 0:
        fps_label = f'FPS: {fps:.2f}'
        cv2.putText(color_image, fps_label, (10, 25), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 0, 255), 2)

    cv2.imshow("output", color_image)

    if cv2.waitKey(1) > -1:
        print("finished by user")
        break

print("Total frames: " + str(total_frames))
