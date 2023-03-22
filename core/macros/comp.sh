read -s password

gnome-terminal --tab -- sshpass -p $password ssh nvidia@192.168.1.204 &
gnome-terminal --tab -- sshpass -p $password ssh nvidia@192.168.1.204 &
gnome-terminal -- sshpass -p $password ssh nvidia@192.168.1.204 &
gnome-terminal -- sshpass -p $password ssh nvidia@192.168.1.204 &
sshpass -p $password ssh nvidia@192.168.1.204
