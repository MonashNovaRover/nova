# Arm input messages
# Each of the inputs range between -1 and 1

# The task velocity is for the IK machine
float32[6] task_velocity

# The joint velocities specifies the movement speed
float32[6] joint_velocity

# End effector values for actuation and linear actuator 

# Input takes value -1, 0, 1
float32 end_effector_actuation
float32 linear_actuation 

# Actuation for lunar construction takes value -1, 0, 1
float32 lunar_construction