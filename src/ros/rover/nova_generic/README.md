# Nova Generic ROS2 Packages

## Overview

This collection of ROS2 packages is designed to provide reusable, configurable code components that can be leveraged across multiple projects and use cases. 
The goal is to reduce the proliferation of custom nodes and interfaces by offering generic interfaces, classes and nodes that are easily customizable through configuration files.

## Current Packages

- [**generic_interfaces:**](./generic_interfaces)  
  Generic message and service definitions for simple message types, providing building blocks for various use cases.

- [**python_control:**](./python_control)  
  A number of classes that can be used to abstract away both high level logic and low level communication with the CAN bus.

- [**python_control_old:**](./python_control_old)  
  An old version of the above used for legacy scraper/tilesplacer code - TODO remove!!!!

## Contribution Criteria

When adding a new package, node, or component to this collection, please ensure it meets the following guidelines:

- **Generic Purpose:**  
  The package should serve a generic purpose that can be reused in a variety of contexts.  
  ✅ Examples: `generic_interfaces`  
  ❌ Not suitable: `ARC_science_messages`

- **Generic Naming:**  
  The name should reflect its broad applicability.  
  ✅ Examples: `generic_can_nodes`  
  ❌ Not suitable: `science_sensors`

If your package does not align with these principles, consider whether it can be refactored to be more generic. 
If it is inherently domain-specific or highly specialized, it is more appropriate to add it to other packages such as `science`, `chassis` or `ec`.

Feel free to reach out on the `#software-help` channel if you need help assessing or refactoring your code to make it generic!

## Development Guidelines

- Aim to maximize code reuse from this collection to maintain consistency and reduce duplication.
- When creating nodes here, please utilize the [generate-parameter-library](https://github.com/ros2/generate_parameter_library) to define clear and maintainable parameters.
- Parameterize nodes extensively to maximize flexibility and adaptability through configuration.
