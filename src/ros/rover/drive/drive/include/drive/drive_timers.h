#include <chrono>



namespace DriveTimers
{
    using namespace std::chrono_literals;
    typedef std::chrono::milliseconds millis;
     
    // Publisher timer periods
    const std::chrono::milliseconds auto_mode        = 200ms;
    const std::chrono::milliseconds drive_control    = 50ms;
    const std::chrono::milliseconds drive_info       = 200ms;
    const std::chrono::milliseconds blcmds_telemetry = 50ms;
    const std::chrono::milliseconds blcmd_spin       = 10ms;
    // Other timer periods
    const std::chrono::milliseconds drive_deadline   = 200ms;
} 