from os import listdir
from os import path

class F:
    """
    Set of colors for printing pretty terminal messages
    """
    D = DELIMITER = "   - "
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
    print(f"{F.G}{F.BOLD}Hello! It seems you are logged into a Jetson!")
    print(f"{F.U}{F.WARNING}Please read ALL of the below before continuing:")
    print("")
    print(f"{F.E}{F.B}{F.D}To check which launch files are running, type {F.C}screen -ls")
    print("")
    print(f"{F.E}{F.B}{F.D}To check on a currently running launch file, type {F.C}check [rover | arm | base]")
    print("")
    print(f"{F.E}{F.B}{F.D}To stop a running service, type {F.C}stop [rover | arm | base]")
    print("")
    print(f"{F.E}{F.B}{F.D}The above commnds will take you to a screen session. {F.WARNING}To EXIT the screen session{F.E}{F.B}, {F.C}press CRTL+A, (release), then press D{F.B}")
    print("")
    print(f"{F.E}{F.B}{F.D}If a screen has crashed and you need to restart, type {F.C}restart [rover | arm | base]")
    print("")


def print_laptop():
    print(f"{F.G}{F.BOLD}Hello! It seems you are logged into a laptop!")
    print("")
    print(f"{F.E}{F.U}{F.WARNING}If you are intending to use this as a base station, make sure to do the following: ")
    print(f"{F.E}{F.E}{F.T}{F.B}1. ssh into to the required jetson")
    print("")
    print(f"{F.E}{F.T}{F.B}2. Follow provided instructions to check the status of running services")
    print("")
    print(f"{F.E}{F.T}{F.B}3. run {F.C}stop base{F.B} to stop base, and run it on your laptop")
    print("")

if __name__ == "__main__":
    if path.isdir("/etc/systemd/system") and 'can.service' in listdir("/etc/systemd/system/"):
        print_jetson()
    else:
        print_laptop()
