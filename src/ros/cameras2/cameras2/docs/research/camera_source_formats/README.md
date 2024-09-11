# Camera source formats

Many cameras support sending video data in multiple formats.  
YUY2 and JPEG are common, with the latter typically offering a higher framerate at the cost of lossy compression.

For example, let's investigate a laptop webcam:

`v4l2-ctl -d /dev/video0 --list-formats-ext`

```
ioctl: VIDIOC_ENUM_FMT
        Type: Video Capture

        [0]: 'MJPG' (Motion-JPEG, compressed)
                Size: Discrete 1280x720
                        Interval: Discrete 0.033s (30.000 fps)
                Size: Discrete 960x540
                        Interval: Discrete 0.033s (30.000 fps)
                Size: Discrete 848x480
                        Interval: Discrete 0.033s (30.000 fps)
                Size: Discrete 640x480
                        Interval: Discrete 0.033s (30.000 fps)
                Size: Discrete 640x360
                        Interval: Discrete 0.033s (30.000 fps)
        [1]: 'YUYV' (YUYV 4:2:2)
                Size: Discrete 640x480
                        Interval: Discrete 0.033s (30.000 fps)
                Size: Discrete 640x360
                        Interval: Discrete 0.033s (30.000 fps)
                Size: Discrete 424x240
                        Interval: Discrete 0.033s (30.000 fps)
                Size: Discrete 320x240
                        Interval: Discrete 0.033s (30.000 fps)
                Size: Discrete 320x180
                        Interval: Discrete 0.033s (30.000 fps)
                Size: Discrete 160x120
                        Interval: Discrete 0.033s (30.000 fps)
                Size: Discrete 1280x720
                        Interval: Discrete 0.100s (10.000 fps)
```

GStreamer can take advantage of both these formats. YUYV is a broadly supported `video/x-raw` format, while MJPG
typically requires decoding.

## Example WebRTC pipelines

### YUY2

`v4l2src device=/dev/video0 ! video/x-raw, format=YUY2 ! webrtcsink`

![](/home/josh/Documents/Code/NovaRover/nova_ws/src/cameras2/cameras2/docs/research/camera_source_formats/yuy2_pipeline.svg)

### YUY2

`v4l2src device=/dev/video0 ! jpegdec ! webrtcsink`

![](/home/josh/Documents/Code/NovaRover/nova_ws/src/cameras2/cameras2/docs/research/camera_source_formats/jpeg_pipeline.svg)