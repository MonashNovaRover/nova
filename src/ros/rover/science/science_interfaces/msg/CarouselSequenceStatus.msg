bool running
int32 current_iteration
int32 total_iterations
string current_step      # 'idle' | 'pumping' | 'moving' | 'delay_pump' | 'delay_move' | 'error' | 'complete'
string ring              # 'inner' | 'outer'
string error_message
