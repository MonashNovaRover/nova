#ifndef LOCKED_PUBLISHER__VISIBILITY_CONTROL_H_
#define LOCKED_PUBLISHER__VISIBILITY_CONTROL_H_

// This logic was borrowed (then namespaced) from the examples on the gcc wiki:
//     https://gcc.gnu.org/wiki/Visibility

#if defined _WIN32 || defined __CYGWIN__
  #ifdef __GNUC__
    #define LOCKED_PUBLISHER_EXPORT __attribute__ ((dllexport))
    #define LOCKED_PUBLISHER_IMPORT __attribute__ ((dllimport))
  #else
    #define LOCKED_PUBLISHER_EXPORT __declspec(dllexport)
    #define LOCKED_PUBLISHER_IMPORT __declspec(dllimport)
  #endif
  #ifdef LOCKED_PUBLISHER_BUILDING_LIBRARY
    #define LOCKED_PUBLISHER_PUBLIC LOCKED_PUBLISHER_EXPORT
  #else
    #define LOCKED_PUBLISHER_PUBLIC LOCKED_PUBLISHER_IMPORT
  #endif
  #define LOCKED_PUBLISHER_PUBLIC_TYPE LOCKED_PUBLISHER_PUBLIC
  #define LOCKED_PUBLISHER_LOCAL
#else
  #define LOCKED_PUBLISHER_EXPORT __attribute__ ((visibility("default")))
  #define LOCKED_PUBLISHER_IMPORT
  #if __GNUC__ >= 4
    #define LOCKED_PUBLISHER_PUBLIC __attribute__ ((visibility("default")))
    #define LOCKED_PUBLISHER_LOCAL  __attribute__ ((visibility("hidden")))
  #else
    #define LOCKED_PUBLISHER_PUBLIC
    #define LOCKED_PUBLISHER_LOCAL
  #endif
  #define LOCKED_PUBLISHER_PUBLIC_TYPE
#endif

#endif  // LOCKED_PUBLISHER__VISIBILITY_CONTROL_H_
