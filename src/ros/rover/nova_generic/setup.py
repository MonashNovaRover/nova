from setuptools import setup
from generate_parameter_library_py.setup_helper import generate_parameter_module

package_name = 'nova_generic'

generate_parameter_module(
  "sensor_parameters", # python module name for parameter library
  "nova_generic/sensors/sensor_parameters.yaml", # path to input yaml file
)

generate_parameter_module(
  "number_sensor_parameters", # python module name for parameter library
  "nova_generic/sensors/number_sensor_parameters.yaml", # path to input yaml file
)

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
    maintainer='Monash Nova Rover',
    maintainer_email='novaroverteam@monash.edu',
    description='Nova Generic',
    license='',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
        ],
    },
)
