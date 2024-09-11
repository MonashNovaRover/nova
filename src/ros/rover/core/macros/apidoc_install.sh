#!/bin/bash
# installs required libraries (on Ubuntu only) for generating docs

sudo apt-get install python3-sphinx
pip install sphinx-autodocgen
pip install sphinx_rtd_theme
