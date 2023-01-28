# This has the raw spectrometer data from the reader.
# It handles all the values from one particular cuvette.

# This defines if the data is valid or not
bool valid

# The current cuvette that has been read (1 - 20)
int32 cuvette

# The current scan of the cuvette (0 - 9)
int32 scan

# The current BCA data
# Should be 6 digits in the array
int32[] bca

# The current FDA data
int32[] fda
