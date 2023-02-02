#include "blcmd.h"
#include <iostream>
#include <string>
#include <unordered_map>

using namespace std;

int main(int argc, char **argv)
{
    BLCMD* blcmd = new BLCMD(2, 1, DRIVE_VELOCITY, false);


    blcmd->drive(2.0);

    string input_command;

    blcmd->stop();

    BLCMDConfig config = blcmd->get_configuration();

    blcmd->set_drive_mode(DRIVE_POSITION);

    blcmd->drive(3.142/2);

    return 0;

}
