#include "print/print.h"
#include "blcmd.h"

int main(int argc, char **argv)
{
    BLCMD* blcmd = new BLCMD(2, 1, DRIVE_VELOCITY, true, STOP, 1);

    blcmd->stop();
    blcmd->forward();
    blcmd->reverse();
    blcmd->drive(0.2);
    // Returns an empty value
    return 0;
}
