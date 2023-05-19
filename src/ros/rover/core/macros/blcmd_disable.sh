# Function to check if a value is a valid boolean
is_boolean() {
  [[ "$1" == true || "$1" == false ]]
}

# Function to check if a value is a valid integer between 1 and 8 (inclusive)
is_integer_between_1_and_8() {
  [[ "$1" =~ ^[1-8]$ ]]
}

# Check if both command-line arguments are provided
if [ $# -ne 2 ]; then
  echo "Usage: $0 <disable_input> <id_input>"
  exit 1
fi

disable_input="$1"
id_input="$2"

# Check if disable_input is a valid boolean
if ! is_boolean "$disable_input"; then
  echo "Invalid disable_input value. It should be either 'true' or 'false'."
  exit 1
fi

# Check if id_input is a valid integer between 1 and 8
if ! is_integer_between_1_and_8 "$id_input"; then
  echo "Invalid id_input value. It should be an integer between 1 and 8 (inclusive)."
  exit 1
fi

# Call the ros2 service with the specified arguments
ros2 service call /control/disable_blcmd core/srv/DisableBLCMD "{disable: $disable_input, id: $id_input}"

