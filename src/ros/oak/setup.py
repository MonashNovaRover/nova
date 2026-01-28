import os
from glob import glob
from setuptools import setup

package_name = "oakenc"

setup(
    name=package_name,
    version="1.0.0",
    packages=[package_name],
    data_files=[
        # https://docs.ros.org/en/foxy/Tutorials/Intermediate/Launch/Launch-system.html
        (os.path.join("share", package_name), glob("launch/*launch.[pxy][yma]*")),
        ("share/ament_index/resource_index/packages", ["resource/" + package_name]),
        ("share/" + package_name, ["package.xml"]),
        *(
            (os.path.join("share", package_name, path), [os.path.join(path, f) for f in files])
            for path, dirs, files in os.walk("params")
        ),
    ],
    install_requires=["setuptools"],
    zip_safe=True,
    maintainer="Monash Nova Rover",
    maintainer_email="novaroverteam@monash.edu",
    description="OAK Camera Server.",
    license="Only for Monash Nova Rover Team to use and distribute.",
    tests_require=["pytest"],
    entry_points={
        "console_scripts": [
            "oak_camera_directory_service = oakenc.oak_camera_directory_service:main",
            #"camera_streamer_service = cameras2.camera_streamer_service:main",
            #"camera_ros_streamer = cameras2.camera_ros_streamer:main",
        ],
    },
)
