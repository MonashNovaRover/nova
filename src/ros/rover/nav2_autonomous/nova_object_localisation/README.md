This package is responsible for object localisation on the Rover.

All packages are designed to be run on the orin, recieving detections from the OAK-D-LR camera and converting them into 3D poses.

More details here: https://www.notion.so/Cube-Detection-19db713961718033b820fc406df6697f

To run, build using NixOS:
```bash
nix-shell -p 'with import /home/USER/nova/nixfiles { }; pkgs.ros.nova-workspace.override {
	novaPackages = {
		inherit (pkgs.ros)
		nova-object-localisation;
	};
}'
```

Then run: `ros2 run nova_object_localisation object_localiser`

Testing in sim:
  Perform object detection on a ros image topic using yolo-ros:
	```nix
	nix-shell -p 'with import /home/nova/nova/nixfiles { }; pkgs.ros.nova-workspace.override {
		novaPackages = {
			inherit (pkgs.ros)
			yolo-bringup;
		};
	}'
	
	ros2 launch yolo_bringup yolo.launch.py
	```
 And `ros2 run nova_object_localisation object_localiser using_oak:=False`

 See here for more details: https://github.com/mgonzs13/yolo_ros

The `auto_bringup` package should have a `yolo.launch.py` file that will launch the most up to date object detection pipeline for both sim and non-sim.

I bet this README will age poorly and not work at all in a few years - Anthony Lew