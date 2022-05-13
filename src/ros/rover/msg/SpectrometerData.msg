# This has the raw spectrometer data from the reader.
# It handles all the values from one particular cuvette.

# This defines if the data is valid or not
bool valid

# The current cuvette that has been read (1 - 20)
int32 cuvette

# The current BCA data
# Each one specifies a list of 10 values from the scans
float32[] bca_0
float32[] bca_1
float32[] bca_2
float32[] bca_3
float32[] bca_4
float32[] bca_5

# The current FDA data
# Each one specifies a list of 10 values from the scans
float32[] fda_0
float32[] fda_1
float32[] fda_2
float32[] fda_3
float32[] fda_4
float32[] fda_5
float32[] fda_6
float32[] fda_7
float32[] fda_8
float32[] fda_9
float32[] fda_10
float32[] fda_11
