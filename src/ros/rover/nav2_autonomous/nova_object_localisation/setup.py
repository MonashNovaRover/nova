from setuptools import setup

package_name = 'nova_object_localisation'

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
    description='Nodes to track cube Marker locations and capture images of placards',
    license='BSD-3-Clause',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            'cube_localiser = nova_object_localisation.cube_localiser:main',
            #'image_capture = archive.image_capture:main'
        ],
    },
)
