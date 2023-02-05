#include "blcmd.h"
#include <iostream>
#include <string>
#include <unordered_map>

using namespace std;

int main(int argc, char **argv)
{
    BLCMD* blcmd = new BLCMD("can1", 1, DRIVE_VELOCITY, false);
	


	if (blcmd->set_config_variable(TELEMETRY_P1_SPEED,0x01)) cout << "PACKET 1 Speed Set" << endl;
	if (blcmd->set_config_variable(TELEMETRY_P2_SPEED, 0x01)) cout << "PACKET 2 Speed Set" << endl;
	if (blcmd->set_config_variable(TELEMETRY_P3_SPEED, 0x01)) cout << "PACKET 3 Speed Set" << endl;
	if (blcmd->set_config_variable(TELEMETRY_P4_SPEED, 0x01)) cout << "PACKET 4 Speed Set" << endl;

	string in;
	blcmd->drive(-0.075);

	BLCMDTelemetry tel = blcmd->get_telemetry();

	cin >> in;
	blcmd->drive(0);

	cout << tel;

    return 0;

}
