from setuptools import find_packages, setup

package_name = 'excavation_construction'

setup(
    name=package_name,
    version='0.0.0',
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
            'scraper = excavation_construction.scraper:main',
            'scraper_old = excavation_construction.scraper_old:main',
            'tile_placer = excavation_construction.tile_placer:main',
            'tile_placer_old = excavation_construction.tile_placer_old:main'
        ],
    },
)
