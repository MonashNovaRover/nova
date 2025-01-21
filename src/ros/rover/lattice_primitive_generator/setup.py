from setuptools import find_packages, setup

package_name = 'lattice_primitive_generator'

setup(
    name=package_name,
    version='1.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='Monash Nova Rover',
    maintainer_email='novaroverteam@monash.edu',
    description='TODO: Package description',
    license='TODO: License declaration',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            'generate_motion_primitives.py = lattice_primitive_generator.generate_motion_primitives:main',
        ],
    },
)
