#ifndef LEGACY_INPUT_MODE__VISIBILITY_CONTROL_H_
#define LEGACY_INPUT_MODE__VISIBILITY_CONTROL_H_

// This logic was borrowed (then namespaced) from the examples on the gcc wiki:
//     https://gcc.gnu.org/wiki/Visibility

#if defined _WIN32 || defined __CYGWIN__
  #ifdef __GNUC__
    #define LEGACY_INPUT_MODE_EXPORT __attribute__ ((dllexport))
    #define LEGACY_INPUT_MODE_IMPORT __attribute__ ((dllimport))
  #else
    #define LEGACY_INPUT_MODE_EXPORT __declspec(dllexport)
    #define LEGACY_INPUT_MODE_IMPORT __declspec(dllimport)
  #endif
  #ifdef LEGACY_INPUT_MODE_BUILDING_LIBRARY
    #define LEGACY_INPUT_MODE_PUBLIC LEGACY_INPUT_MODE_EXPORT
  #else
    #define LEGACY_INPUT_MODE_PUBLIC LEGACY_INPUT_MODE_IMPORT
  #endif
  #define LEGACY_INPUT_MODE_PUBLIC_TYPE LEGACY_INPUT_MODE_PUBLIC
  #define LEGACY_INPUT_MODE_LOCAL
#else
  #define LEGACY_INPUT_MODE_EXPORT __attribute__ ((visibility("default")))
  #define LEGACY_INPUT_MODE_IMPORT
  #if __GNUC__ >= 4
    #define LEGACY_INPUT_MODE_PUBLIC __attribute__ ((visibility("default")))
    #define LEGACY_INPUT_MODE_LOCAL  __attribute__ ((visibility("hidden")))
  #else
    #define LEGACY_INPUT_MODE_PUBLIC
    #define LEGACY_INPUT_MODE_LOCAL
  #endif
  #define LEGACY_INPUT_MODE_PUBLIC_TYPE
#endif

#endif  // LEGACY_INPUT_MODE__VISIBILITY_CONTROL_H_
