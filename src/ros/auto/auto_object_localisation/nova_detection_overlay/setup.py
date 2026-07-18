from setuptools import setup

package_name = 'nova_detection_overlay'

setup(
    name=package_name,
    version='0.0.0',
    packages=[package_name],
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='nova',
    maintainer_email='novaroverteam@monash.edu',
    description='Custom ROS2 node that publishes YOLO model detection overlay',
    license='BSD-3-Clause',
    entry_points={
        'console_scripts': [
            'detection_overlay = nova_detection_overlay.detection_overlay:main'
        ],
    },
)
