# Nova Generic ROS2 Packages

## Overview

This collection of ROS2 packages is designed to provide reusable, configurable code components that can be leveraged across multiple projects and use cases. 
The goal is to reduce the proliferation of custom nodes by offering generic nodes that are easily customizable through configuration files.

## Contribution Criteria

When adding a new package, node, or component to this collection, please ensure it meets the following guidelines:

- **Generic Purpose:**  
  The package should serve a generic purpose that can be reused in a variety of contexts.  
  ✅ Examples: `generic_interfaces`, `generic_CAN_nodes`  
  ❌ Not suitable: domain-specific messages such as `ARC_science_messages`

- **Generic Naming:**  
  The name should reflect its broad applicability.  
  ✅ Examples: `generic_can_nodes`  
  ❌ Not suitable: `science_sensors`

If your package does not align with these principles, consider whether it can be refactored to be more generic. 
If it is inherently domain-specific or highly specialized, it is more appropriate to add it to other packages such as `nova-interfaces`, `science`, `electrical`, or `ec`.

Feel free to reach out on the `#software` channel if you need help assessing or refactoring your code.

## Development Guidelines

- Aim to maximize code reuse from this collection to maintain consistency and reduce duplication.
- When creating nodes here, please utilize the [generate-parameter-library](https://github.com/ros2/generate_parameter_library) to define clear and maintainable parameters.
- Parameterize nodes extensively to maximize flexibility and adaptability through configuration.

## Current Packages

- **generic_can_nodes:**  
  Generic nodes for interfacing with the CAN bus in straightforward scenarios such as listening and publishing.

- **generic_interfaces:**  
  Generic message and service definitions for simple message types, providing building blocks for various use cases.
