#!/usr/bin/env python3

# Include the setup tools
from setuptools import setup

# Setup the package
setup(
    name="can_sleuth",
    version="0.0.1",
    author="Monash Nova Rover",
    author_email="novaroverteam@monash.edu",
    description="CAN device simulator/tracer.",
    long_description="TODO",
    long_description_content_type="text/markdown",
    url="https://github.com/MonashNovaRover/nova",
    packages=['can_sleuth', 'can_sleuth.outputs', 'can_sleuth.devices'],
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: Apache 2.0",
        "Operating System :: OS Independent",
    ],
    install_requires=[
        'jcan',
        'pyside6'
        ],
    python_requires='>=3.10',
    scripts=["scripts/can_sleuth"],
)
