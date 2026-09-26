# Exit on error
set -e

cd firmware/src # Go to firmware directory
make clean
make            # Make .o files
make lib        # Make library (libnetat_api.so)

python3 ../../software/test/libnetat_terminal.py # Step 3: Run the Python script from the correct relative path
