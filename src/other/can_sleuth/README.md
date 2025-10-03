# CAN sleuth

A general purpose low level simulator/sniffer for devices (particularly CAN)
that allows their state to be presented is a readable/useful manner.

At the top we have the Manager, which then has a list of outputs and a list of
devices as children. It will tell the devices to update their state, represented
by a collection of "attributes", and then the outputs to display the state of
attributes of all the devices (be it to a terminal, a csv file, or whatever).

Helper functions for creating all the devices in real systems/payloads are in
`systems.py`.

Usage:

```
$ ./main.py --help
usage: ./main.py [[-e | --emulate] [{-i | --interface} INTERFACE ] PAYLOAD]

Available Payloads/Systems: drive taipan drive25_26 taipan_spherical

E.g. to emulate+trace taipan:       `./main.py -e taipan`
     to trace drive on can2:        `./main.py --interface can2 drive`
     to emulate+trace drive+taipan: `./main.py -e drive -e taipan`
```

Output
- Terminal Curses TUI
- CSV (just an idea)
- json (just an idea)

Main Runner

Devices
- BLCMD
- CMD (not implemented)
- Arm baseboard (not implemented)
- QCMD (not implemented)
- LED Driver (not implemented)
- Kiln (not implemented)
- Misc Science stuff (not implemented)
- Battery (not implemented)

Systems (payloads):
- Taipan Arm
- Old Arm (not implemented)
- Drive
- Scraper (not implemented)
- Tile Placer (not implemented)
- Science 25/26 (not implmented)

Devices have several attributes that can be logged/displayed

Attributes have:
- name
- width/height (for when displayed as text)
- units
- data
- raw data (just an idea)
- importance (just an idea) (i.e. important attrs always shown, but if you select one device you view all attrs.)

```
┌<J1>──────────────────────────┐┌<J3>──────────────────────────┐┌<J5>──────────────────────────┐
│velocity: 0                   ││velocity: 0                   ││velocity: 0                   │
│Qcurrent: 0                   ││Qcurrent: 0                   ││Qcurrent: 0                   │
│interval: None                ││interval: None                ││interval: None                │
│Dcurrent: None                ││Dcurrent: None                ││Dcurrent: None                │
│resolverPosition: 7951        ││resolverPosition: 19830       ││resolverPosition: 57271       │
│resolverVelocity: 31804       ││resolverVelocity: 14038       ││resolverVelocity: 33241       │
│power: None                   ││power: None                   ││power: None                   │
│voltage: None                 ││voltage: None                 ││voltage: None                 │
│temperature: None             ││temperature: None             ││temperature: None             │
│current: None                 ││current: None                 ││current: None                 │
│err: None                     ││err: WARN: MAGNETIC_ENCODER   ││err: None                     │
│msg: twitchFw                 ││msg: twitchFw                 ││msg: twitchBk                 │
└──────────────────────────────┘└──────────────────────────────┘└──────────────────────────────┘
┌<J2>──────────────────────────┐┌<J4>──────────────────────────┐┌<J6>──────────────────────────┐
│velocity: 0                   ││velocity: 0                   ││velocity: 0                   │
│Qcurrent: 0                   ││Qcurrent: 0                   ││Qcurrent: 0                   │
│interval: None                ││interval: None                ││interval: None                │
│Dcurrent: None                ││Dcurrent: None                ││Dcurrent: None                │
│resolverPosition: 35          ││resolverPosition: 57193       ││resolverPosition: 257         │
│resolverVelocity: 140         ││resolverVelocity: 32930       ││resolverVelocity: 1028        │
│power: None                   ││power: None                   ││power: None                   │
│voltage: None                 ││voltage: None                 ││voltage: None                 │
│temperature: None             ││temperature: None             ││temperature: None             │
│current: None                 ││current: None                 ││current: None                 │
│err: INFO: RESOLVER           ││err: GATE: RESOLVER_ZERO      ││err: None                     │
│msg: twitchBk                 ││msg: twitchBk                 ││msg: Pos: 0x0102              │
└──────────────────────────────┘└──────────────────────────────┘└──────────────────────────────┘
```
