#ifndef AUGER_CONTROLLER__VISIBILITY_CONTROL_H_
#define AUGER_CONTROLLER__VISIBILITY_CONTROL_H_

// This logic was borrowed (then namespaced) from the examples on the gcc wiki:
//     https://gcc.gnu.org/wiki/Visibility

#if defined _WIN32 || defined __CYGWIN__
  #ifdef __GNUC__
    #define AUGER_CONTROLLER_EXPORT __attribute__ ((dllexport))
    #define AUGER_CONTROLLER_IMPORT __attribute__ ((dllimport))
  #else
    #define AUGER_CONTROLLER_EXPORT __declspec(dllexport)
    #define AUGER_CONTROLLER_IMPORT __declspec(dllimport)
  #endif
  #ifdef AUGER_CONTROLLER_BUILDING_LIBRARY
    #define AUGER_CONTROLLER_PUBLIC AUGER_CONTROLLER_EXPORT
  #else
    #define AUGER_CONTROLLER_PUBLIC AUGER_CONTROLLER_IMPORT
  #endif
  #define AUGER_CONTROLLER_PUBLIC_TYPE AUGER_CONTROLLER_PUBLIC
  #define AUGER_CONTROLLER_LOCAL
#else
  #define AUGER_CONTROLLER_EXPORT __attribute__ ((visibility("default")))
  #define AUGER_CONTROLLER_IMPORT
  #if __GNUC__ >= 4
    #define AUGER_CONTROLLER_PUBLIC __attribute__ ((visibility("default")))
    #define AUGER_CONTROLLER_LOCAL  __attribute__ ((visibility("hidden")))
  #else
    #define AUGER_CONTROLLER_PUBLIC
    #define AUGER_CONTROLLER_LOCAL
  #endif
  #define AUGER_CONTROLLER_PUBLIC_TYPE
#endif

#endif  // AUGER_CONTROLLER__VISIBILITY_CONTROL_H_
