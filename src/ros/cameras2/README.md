# 📷 cameras2 📷

This project provides camera discovery and streaming services, and is designed with the following goals in mind:

- **Resiliency**: Variable bitrate streams are supported, and packet loss is handled gracefully.
- **Portability**: As few assumptions as possible are made about the camera hardware. New cameras should work with 
                   minimal effort.
- **Web support**: Camera footage can be displayed in a web browser - live and with no transcoding.

## Usage
### Discovery
The [`camera_directory_service`](./cameras2/cameras2/camera_directory_service.py) exposes connected camera information
over ROS. It can be used to find cameras connected to the rover.

```shell
ros2 topic echo /camera_directory/cameras --qos-reliability reliable --qos-durability transient_local
```

Take a look at the [`camera_streamer_service`](./cameras2/cameras2/camera_streamer_service.py) for an example usage in
Python.

### Streaming
#### Backend
The [`camera_streamer_service`](./cameras2/cameras2/camera_streamer_service.py) provides WebRTC streams of camera
footage.

The following documentation details the manual setup required to run the backend. This process will be streamlined in
the future.

##### Dependencies
###### Python packages
- [pyudev](https://pypi.org/project/pyudev)
- [PyGObject](https://pypi.org/project/PyGObject)

###### Libraries
- [GStreamer](https://gstreamer.freedesktop.org/)
- [GStreamer Base Plugins](https://gstreamer.freedesktop.org/modules/gst-plugins-base.html)
- [GStreamer Good Plugins](https://gstreamer.freedesktop.org/modules/gst-plugins-good.html)
- [GStreamer Bad Plugins](https://gstreamer.freedesktop.org/modules/gst-plugins-bad.html)
- [GStreamer Ugly Plugins](https://gstreamer.freedesktop.org/modules/gst-plugins-ugly.html)
- [gst-plugin-webrtc](https://gitlab.freedesktop.org/gstreamer/gst-plugins-rs/-/tree/main/net/webrtc)
- [gst-plugin-rtp](https://gitlab.freedesktop.org/gstreamer/gst-plugins-rs/-/tree/main/net/rtp)
- [Nice: GLib ICE library](https://github.com/libnice/libnice)

###### Executables
- [gst-plugin-webrtc-signalling](https://gitlab.freedesktop.org/gstreamer/gst-plugins-rs/-/tree/main/net/webrtc/signalling)
- [stunserver](https://github.com/jselbie/stunserver)

##### Execution
1. WebRTC needs an independent "signalling server". The intended server for this project is
   [gst-plugin-webrtc-signalling](https://gitlab.freedesktop.org/gstreamer/gst-plugins-rs/-/tree/main/net/webrtc/signalling).

   ```shell
   # On the rover
   gst-webrtc-signalling-server
   ```

2. Some browsers require a STUN server for ICE, even though everything is on a LAN.
   A server such as [stunserver](https://github.com/jselbie/stunserver) should be run.
   ```shell
   # On the base station
   stunserver
   ```

3. Start the service.
   
   ```shell
   # On the rover

   # Make sure the camera directory service is running
   ros2 run cameras2 camera_directory_service

   ros2 run cameras2 camera_streamer_service
   ```

4. Start streaming.  
   Note: Tab-completion or the [`camera_directory_service`](./cameras2/cameras2/camera_directory_service.py) can be used
   to find a camera serial number. If no serial numbers are given, all connected cameras will be operated on.
   ```shell
   ros2 service call /camera_streamer/stream/start camera_msgs/srv/CameraOperation '{ serials: [ ] }'
   ```

#### Frontend
Integration with the [GUI](https://github.com/MonashNovaRover/gui) is not yet complete.
For now, the [example Web app](https://gitlab.freedesktop.org/gstreamer/gst-plugins-rs/-/tree/0.10.2/net/webrtc/www) included in the
[gst-plugin-webrtc](https://gitlab.freedesktop.org/gstreamer/gst-plugins-rs/-/tree/main/net/webrtc) repository can be
used.

```shell
# On the rover
git clone https://gitlab.freedesktop.org/gstreamer/gst-plugins-rs.git
python3 -m http.server -d gst-plugins-rs/net/webrtc/www
```

```shell
# On the base station
# Replace <rover_ip> with the rover's IP address (typically 192.168.0.204)
xdg-open http://<rover_ip>:8000
```

## Technical notes
### Research
Research and development notes on various topics can be found in the [research](./cameras2/docs/research) directory.

### Camera identification
While cameras are ideally identified by serial number (and the APIs provided by the packages in this repository are
designed with this in mind), some USB webcams do not use a unique serial value in their USB descriptors.

In this case, the USB port in use is used as an identifier, and mapped to a fake unique serial number. This behaviour
is implemented in [`cameras.py`](./cameras2/cameras2/cameras.py).
