This file contains every tunable property. They can be applied in any order, and any undeclared property will be undeclared.

This file is read in runtime, every time the camera pipeline is started. It is possible to crash cameras3 if an invalid property value is given, such as loading unsupported resolutions.

If the camera only supports 30fps and the user requests 20fps, it will segfault.

During deployment, it is recommended to only change camera options via profiles.


Tunable parameters are found in include/cameras/cameras.hpp, and their valid values can be found on the gstreamer plugin wiki.


The priority of parameters read is:
serial_overrides -> profiles -> default_values

serial_overrides and profiles are both defined in streamer.yaml, while default_values are hard-coded sane defaults that will function even without a valid parameters directory.