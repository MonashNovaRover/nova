from setuptools import setup

package_name = 'nova_ar_tag'

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
    description='Custom ROS2 node that publishes AR tag markers',
    license='BSD-3-Clause',
    entry_points={
        'console_scripts': [
            'aruco_marker = nova_ar_tag.aruco_marker:main',
            'marker_distance = nova_ar_tag.marker_distance:main'
        ],
    },
)
