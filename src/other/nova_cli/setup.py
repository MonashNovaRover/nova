#!/usr/bin/env python3
"""Setup script for nova_cli package"""

from setuptools import setup, find_packages

setup(
    name="nova_cli",
    version="1.0.0",
    author="Monash Nova Rover",
    author_email="novaroverteam@monash.edu",
    description="Nova ROS2 CLI wrapper tool",
    long_description="CLI wrapper for ROS2 launch and run commands with automatic transformations and build selection",
    long_description_content_type="text/markdown",
    url="https://github.com/MonashNovaRover/nova",
    packages=find_packages(),
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: Apache Software License",
        "Operating System :: OS Independent",
    ],
    install_requires=[
        'argcomplete',  # For bash completion
    ],
    python_requires='>=3.10',
    entry_points={
        'console_scripts': [
            'nova=nova_cli.main:main',
        ],
    },
)
