#include "blcmd.h"
#include <iostream>
#include <string>
#include "jcan.h"

using namespace std;

int main(int argc, char **argv) {
    BLCMD *blcmd = new BLCMD("vcan0", 1, DRIVE_VELOCITY, false);



//	if (blcmd->set_config_variable(TELEMETRY_P1_SPEED,0x01)) cout << "PACKET 1 Speed Set" << endl;
//	if (blcmd->set_config_variable(TELEMETRY_P2_SPEED, 0x01)) cout << "PACKET 2 Speed Set" << endl;
//	if (blcmd->set_config_variable(TELEMETRY_P3_SPEED, 0x01)) cout << "PACKET 3 Speed Set" << endl;
//	if (blcmd->set_config_variable(TELEMETRY_P4_SPEED, 0x01)) cout << "PACKET 4 Speed Set" << endl;

    blcmd->drive(-0.075);

    BLCMDTelemetry tel;

    std::vector<bool> recieved = blcmd->get_telemetry(&tel);

    cout << "Recieved Packet 1: " << recieved[0] << endl;
    cout << "Recieved Packet 2: " << recieved[1] << endl;
    cout << "Recieved Packet 3: " << recieved[2] << endl;
    cout << "Recieved Packet 4: " << recieved[3] << endl;

    cout << tel;

    return 0;

}