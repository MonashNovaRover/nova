# core :scroll:

This is the core repository and should be installed on every Nova device. It contains all bash scripts, alias, schema and ROS 2 custom interfaces. No ROS code, excluding interfaces, should exist on this repository.

---

### Commands

The core repository comes with some useful commands, all of which are found inside the **alias.sh** file. Please check them out and become familiar with some of the commands. Here are the key ones:

- **build**: Builds all ROS 2 packages inside of the nova workspace. Can be run from any directory on a device, and does not need to be run from the workspace folder.

- **pull**: Pulls all repository code inside the src directory in the nova workspace. Can be run from any directory and will pull the current working branches.

- **setup**: In some cases, ROS 2 may require you to setup the package again. This command can run this program from any directory.

- **nova**: Changes directory to the nova workspace from any directory on the device.

---

### Setup
Please add the **nova.sh** bash script to your bashrc.

``sudo echo "source ~/nova_ws/src/core/nova.sh" < ~/.bashrc``

This will run the main bash script after every new Terminal instance. These will be required to run the bash scripts.
Alternatively, to install all packages, dependencies and repositories to your Linux device, run the following bash script:

``sudo ./ core/macros/setup.sh``

Be aware,t his script will remove the current repositories and replace them all, along with all the macros. Make sure you have no local commits prior to running this line of code.

---

## Interfaces

This repository stores all of the relevant messages, services and action interface classes. These are custom made interfaces that do not have an identical class inside the std_msgs library. Before creating a new interface, please make sure this is the case.

---

#### Messages

**Template.msg**
Inputs:
> string param1

> int64 param2

<br/>

---


#### Services

**Template.srv**
Inputs:
> string param1

> int64 param2

Outputs:
> string param3

<br/>

---


#### Actions

**Template.action**
Inputs:
> string param1

> int64 param2

Outputs:
> string param3

Feedback:
> float64 param4

<br/>

---
