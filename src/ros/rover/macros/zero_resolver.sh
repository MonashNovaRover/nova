#!/bin/bash

# +--------------------------------------------+
#               MONASH NOVA ROVER
# +--------------------------------------------+
#
# This script allows users to call the ROS2 service
#   to zero resolvers on the arm
#
# Usgae:
#
# > zero_resolver [JOINT_NAME]
#
# Joint names can include base-rotation, shoulder, elbow, j4, etc
#
# +--------------------------------------------+

# Add the colors
ERROR='\033[0;31;1m'
END='\033[0m'

# Create an error function
error () {
    printf "${ERROR}${1}${END}\n"
}

# Checks for missing keyword
if [[ -z $1 ]]
then
    # Display error
    error "Missing command. Specify which joint to zero"
else
    # Call the service with the given joint
    ros2 service call electronics/resolver_zero_service core/srv/StringTrigger "{value: '$1'}"
fi
