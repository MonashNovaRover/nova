// Copyright 2025 Bailey Chessum
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
#ifndef JOINT_SPACE_CONTROL_MODE__VISIBILITY_CONTROL_H_
#define JOINT_SPACE_CONTROL_MODE__VISIBILITY_CONTROL_H_

// This logic was borrowed (then namespaced) from the examples on the gcc wiki:
//     https://gcc.gnu.org/wiki/Visibility

#if defined _WIN32 || defined __CYGWIN__
  #ifdef __GNUC__
    #define JOINT_SPACE_CONTROL_MODE_EXPORT __attribute__ ((dllexport))
    #define JOINT_SPACE_CONTROL_MODE_IMPORT __attribute__ ((dllimport))
  #else
    #define JOINT_SPACE_CONTROL_MODE_EXPORT __declspec(dllexport)
    #define JOINT_SPACE_CONTROL_MODE_IMPORT __declspec(dllimport)
  #endif
  #ifdef JOINT_SPACE_CONTROL_MODE_BUILDING_LIBRARY
    #define JOINT_SPACE_CONTROL_MODE_PUBLIC JOINT_SPACE_CONTROL_MODE_EXPORT
  #else
    #define JOINT_SPACE_CONTROL_MODE_PUBLIC JOINT_SPACE_CONTROL_MODE_IMPORT
  #endif
  #define JOINT_SPACE_CONTROL_MODE_PUBLIC_TYPE JOINT_SPACE_CONTROL_MODE_PUBLIC
  #define JOINT_SPACE_CONTROL_MODE_LOCAL
#else
  #define JOINT_SPACE_CONTROL_MODE_EXPORT __attribute__ ((visibility("default")))
  #define JOINT_SPACE_CONTROL_MODE_IMPORT
  #if __GNUC__ >= 4
    #define JOINT_SPACE_CONTROL_MODE_PUBLIC __attribute__ ((visibility("default")))
    #define JOINT_SPACE_CONTROL_MODE_LOCAL  __attribute__ ((visibility("hidden")))
  #else
    #define JOINT_SPACE_CONTROL_MODE_PUBLIC
    #define JOINT_SPACE_CONTROL_MODE_LOCAL
  #endif
  #define JOINT_SPACE_CONTROL_MODE_PUBLIC_TYPE
#endif

#endif  // JOINT_SPACE_CONTROL_MODE__VISIBILITY_CONTROL_H_
