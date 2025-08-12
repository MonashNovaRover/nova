# nova_detection_overlay
Don't let the obscene file structure scare you!
This is literally a giant wrapper for a very simple python file which runs a ros2 node which takes the detections ROS2 messages by the OAK-D-LR camera and overlays the coordinates as rectangles onto the video feed then publishes them to a new topic.

It used to be in auto_utils *(formerly nova_utils)* but then someone moved it into here.

The aim is to make this more versile and support more detection formats robustly so that object debugging code will go into this package!

`ros2 run nova_detection_overlay detection_overlay.py` 
*tbh idk if this command works, if it doesnt try the following instead:*
```
ws-shell
cd ./nova_detection_overlay # assuming you're in this readme's folder
python detection_overlay.py
```

Subscriptions:
- `/oak/rgb/preview/image_raw` - Type [`sensor_msgs/msgs/Image`](https://docs.ros.org/en/jade/api/sensor_msgs/html/msg/Image.html)
- `/oak/nn/detections` - Type [`vision_msgs/msgs/Detection2DArray`](https://docs.ros.org/en/noetic/api/vision_msgs/html/msg/Detection2DArray.html)
- `/oak/nn/spatial_detections` - Type [`vision_msgs/msgs/Detection3DArray`](https://docs.ros.org/en/noetic/api/vision_msgs/html/msg/Detection3DArray.html)
Publishers:
- `/overlay` - Type [`sensor_msgs/msgs/Image`](https://docs.ros.org/en/jade/api/sensor_msgs/html/msg/Image.html)
