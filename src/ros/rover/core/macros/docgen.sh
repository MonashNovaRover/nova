#!/bin/bash

# Builds Documentation and opens the home page (index.html) in Firefox

# You will need to have installed the following:
# sudo apt-get install python3-sphinx 
# pip install sphinx-autodocgen 
# pip install sphinx_rtd_theme

cd ~/nova_ws/src/rover/

for module in autonomous  control  core  electronics  science; do
    sphinx-apidoc -o ~/nova_ws/src/rover/docs/source $module
done

cd ~/nova_ws/src/rover/docs/
make html

firefox build/html/index.html
