# message for broadcasting log messages (e.g. errors, warnings, info messages, etc.)

# everything is of type string (not number) as I do not trust
# us to maintain a consistent standard for numerical ids

# source of logs (e.g. source = "Front Left Wheel BLCMD")
string source

# lists of log messages, sorted into their categories (e.g. errors = ["Resolver Fault"])
string[] errors
string[] warnings
string[] info