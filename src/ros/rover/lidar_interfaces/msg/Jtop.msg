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

# CPU Cores
int32[] cpu_core_freq   # current cpu core frequencies
float32[] cpu_core_idle # current % of time spent idle for all cpu cores

# GPU
int32 gpu_freq    # current gpu frequency
float32 gpu_load  # current usage of the gpu

# Power
int32 voltage  # voltage in millivolts
int32 current  # current in milliamperes
int32 power    # power in milliwatts
int32 power_avg   # power average in milliwatts???

# Disk I/O (KB/s)
int32 disk_read
int32 disk_write

# Network I/O (KB/s)
int32 net_tx
int32 net_rx
