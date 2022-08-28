from os import listdir
from os import path

class F:
    """
    Set of colors for printing pretty terminal messages
    """
    D = DELIMITER = "   > "
    T = DELIMITER = "     "
    H = HEADER = '\033[95m'
    B = OKBLUE = '\033[94m'
    C = OKCYAN = '\033[96m'
    G = OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    U = UNDERLINE = '\033[4m'
    E = ENDUNDERLINE = '\033[24m'


def print_jetson():
    print("")
    print("")
    print(f"{F.G}{F.BOLD}Hello! It seems you are logged into a Jetson!")
    print("")
    print(f"{F.U}{F.WARNING}Please read ALL of the below before continuing...")
    print(f"{F.E}{F.T}{F.B}To check the currently running rover launch file, type {F.C}check rover")
    print(f"{F.T}{F.B}To check the currently running arm launch file, type {F.C}check arm")
    print(f"{F.T}{F.B}To check the currently running base launch file, type {F.C}check base")
    print("")
    print(f"{F.B}The above commnds will take you to a screen session. To exit the screen session, use {F.C}CRTL+A, D{F.B}")
    print(f"{F.B}If any of these don't take you to a new screen, they have probably crashed and will need to be run again!")
    print(f"{F.B}Simply type {F.C}restart system{F.B} (where system is either {F.C}rover, arm or base{F.B})")
    print("")
    print("")


def print_laptop():
    print("")
    print("")
    print(f"{F.G}{F.BOLD}Hello! It seems you are logged into a laptop!")
    print("")
    print(f"{F.U}{F.WARNING}If you are intending to use this as a base station, make sure to do the following: ")
    print(f"{F.E}{F.T}{F.B}1. ssh into to the required jetson")
    print(f"{F.T}{F.B}2. Follow provided instructions to check the status of running services")
    print(f"{F.T}{F.B}3. run {F.C}stop_base{F.B} to stop base, and run it on your laptop")
    print("")
    print("")

if __name__ == "__main__":
    if path.isdir("/etc/systemd/system") and 'can.service' in listdir("/etc/systemd/system/"):
        print_jetson()
    else:
        print_laptop()
