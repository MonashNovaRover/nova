# Nova CAN Database

Hello intrepid adventurer, for you have arrived upon strange and unknown shores. This Nix package contains all files that generate the CAN database files (basically URDF files, but for CAN) you can find at the `can-database` subdirectory in each build of `nova-workspace` (i.e. created by running `ws-build`). These CAN Database files can then be provided to industry standard CAN analysis software to decode the meaning behind the messages seen on a CAN bus (and then do cool things like graph those interpreted values!).

### Using Nova CAN Database

View the `can-database` subdirectory in your latest build to see what rovers and rover systems are supported. You can either use `cantools` (e.g. with `monitor` or `decode`) or SavvyCAN (gui application) to analyse the CAN bus with these database files. Aliases for `cantools monitor` are provided for your convenience:
```shell
monitor-can0          # for the drive system on can0
monitor-can1-taipan   # for Taipan (new arm) on can1
```

Usually, CAN database files are provided with 2 different variants, e.g. `can0.dbc` and `can0_interpreted.dbc`. CAN database files allow us to define constants which enable software to automatically convert raw CAN data into values (e.g. with SI units like `deg/s`), or convert them to be a proportion of the maximum possible value (e.g. for effort). Interpreted CAN database files _attempt_ to do this conversion, however the accuracy is not guaranteed (and is often highly uncertain with firmware changes). Calculations to determine these magic constants are derived principally from `blcmd_hardware2.cpp` and `ros2_control.xacro`.

You may also use `cannelloni` to "transfer/send" a CAN bus over the network to your laptop (e.g. running virtual can), which can then be analysed using `cantools` or SavvyCAN (for superior performance compared to using ssh with X11 forwarding; i.e. running either tool directly in a ssh terminal connected to the rover).

### Build Process

The generation of these files follows a 2 step process. Firstly, files with the `.kcd.xacro` extension are processed by the `xacro` tool to evaluate all calculations and imported XML; this generates a valid `.kcd` file, which is a CAN database format. However, only few software supports this format, so a second step to convert it into a `.dbc` file is required using `cantools convert`; this is supported by nearly all software as it's the industry standard. It is also worth noting that all calculations to determine magic constants used to interpret raw CAN data are performed by `xacro`, with its result hard-coded into the resulting `.kcd` file.

### Directory Structure

```shell
can_database/
├── banksia # all CAN database files for this rover
│   ├── boards # macros that define messages for each type of electrical board
│   │   ├── blcmd_interpreted.kcd.xacro
│   │   ├── blcmd.kcd.xacro
│   │   └── qcmd.kcd.xacro
│   ├── misc # global node definition macro and templates (for you to use)
│   │   ├── macro_template.kcd.xacro
│   │   ├── network_template.kcd.xacro
│   │   └── nodes.kcd.xacro
│   ├── networks # usually each include a single system macro to define all messages on a single CAN bus
│   │   │ # usually 1 for each CAN bus configuration (e.g. can1 with Taipan)
│   │   │ # with "interpreted" variant (i.e. instead of reporting raw values, it attempts to convert them into SI units like radians per second)
│   │   ├── can0_interpreted.kcd.xacro
│   │   ├── can0.kcd.xacro
│   │   ├── can1_taipan_interpreted.kcd.xacro
│   │   └── can1_taipan.kcd.xacro
│   └── systems # macros built by including board (and some misc) macros to create a set of defined CAN messages for each "system" (e.g. payload)
│       ├── drive_interpreted.kcd.xacro
│       ├── drive.kcd.xacro
│       ├── taipan_interpreted.kcd.xacro
│       └── taipan.kcd.xacro
├── default.nix
├── nix
│   └── default.nix # Nix package defintion
└── schemas # contains explanation on using XML schemas with Nova CAN Database

```

_Hope this helps! - Jonathan Jia_