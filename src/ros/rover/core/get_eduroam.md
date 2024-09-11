### To get eduroam, you need to do network things


First, run 

- `ip route`

This will show you a default route as the first thing in a list of things, starting with `eth 0` or something 

Type 
`sudo ip route del default via 192.168.1.1 dev eth0 proto static metric 20100`
or
`sudo ip route del <whatever is the first line in that last above>`

Then, if there is an apt lock issue, run:

`sudo systemctl stop apt-daily-upgrade.service`
