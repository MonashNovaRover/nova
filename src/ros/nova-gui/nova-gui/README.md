## Nova-GUI Frontend

This directory is the root directory of the drontend that constitutes Nova-GUI. Basic terminology and Best practices are outlined below.

### Folder Structure

<!-- Taken from PR #1 -->

The Folder Structure's been Organised into the following

- **assets**: Store all non typescript stuff such as Images, Fonts and other misc stuff
- **routes**: Store all routing related stuff
- **ros**: Store ROS Related stuff. The ROS TS Generator would output to this folder.
- **redux**: All Redux and Bifrost Related Stuff
- **views**: Views in this case refers to the different "Profiles". Each view will contain It's Own Layouts and other components which are specific to the purpose.
- **components**: This is our effective component library containing all components used.

### Component Development Lifecycle

Designing and Drafting a Component can include a myriad of Confusion and Ambiguity. This section aims at making this a bit more easy for people to onboard and design their components.

All components on the Components Folder needs to be Generic and Monolithic in Nature, i,e one should be able to use this components across views without any difficulty. All Hooks, functions, enums and types should be stored in the component folder. It's recommended to substructure a component if needed.The shared folder houses components that is used in other components.

A component can do one or more of the following functions

- Stream data from the Rover
- Recieve on demand data from the Rover (like a REST API)
- Send a command to the Rover to perform a Task
- Please the Rover Operator by looking Cute and Cheerfull
