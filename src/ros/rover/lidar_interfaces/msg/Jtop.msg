std_msgs/Header header

# Temp
float32 temp_cpu
float32 temp_cv0
float32 temp_cv1
float32 temp_cv2
float32 temp_gpu
float32 temp_soc1
float32 temp_soc2
float32 temp_tj

# Memory
int32 ram_tot
int32 ram_used
int32 swap_tot
int32 swap_used

# CPU
float32 cpu_total_user  # total % spent for user-space
float32 cpu_total_nice
float32 cpu_total_system
float32 cpu_total_idle  # total % unused
float32 cpu_total       # total % usage

# CPU Cores
int32[] cpu_core_freq   # current cpu core frequencies
float32[] cpu_core_total # current % of time spent working for all cpu cores

# GPU
int32 gpu_freq    # current gpu frequency
float32 gpu_load  # current usage of the gpu

# Power
int32 voltage  # voltage in millivolts
int32 current  # current in milliamperes
int32 power    # power in milliwatts
int32 power_avg   # power average in milliwatts???

# Disk I/O (KB/s)
float32 disk_read
float32 disk_write

# Network I/O (KB/s)
float32 net_tx
float32 net_rx
