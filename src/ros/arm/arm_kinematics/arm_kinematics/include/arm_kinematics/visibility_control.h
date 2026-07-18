#ifndef ARM_KINEMATICS__VISIBILITY_CONTROL_H_
#define ARM_KINEMATICS__VISIBILITY_CONTROL_H_

// This logic was borrowed (then namespaced) from the examples on the gcc wiki:
//     https://gcc.gnu.org/wiki/Visibility

#if defined _WIN32 || defined __CYGWIN__
#ifdef __GNUC__
#define ARM_KINEMATICS_EXPORT __attribute__((dllexport))
#define ARM_KINEMATICS_IMPORT __attribute__((dllimport))
#else
#define ARM_KINEMATICS_EXPORT __declspec(dllexport)
#define ARM_KINEMATICS_IMPORT __declspec(dllimport)
#endif
#ifdef ARM_KINEMATICS_BUILDING_DLL
#define ARM_KINEMATICS_PUBLIC ARM_KINEMATICS_EXPORT
#else
#define ARM_KINEMATICS_PUBLIC ARM_KINEMATICS_IMPORT
#endif
#define ARM_KINEMATICS_PUBLIC_TYPE ARM_KINEMATICS_PUBLIC
#define ARM_KINEMATICS_LOCAL
#else
#define ARM_KINEMATICS_EXPORT __attribute__((visibility("default")))
#define ARM_KINEMATICS_IMPORT
#if __GNUC__ >= 4
#define ARM_KINEMATICS_PUBLIC __attribute__((visibility("default")))
#define ARM_KINEMATICS_LOCAL __attribute__((visibility("hidden")))
#else
#define ARM_KINEMATICS_PUBLIC
#define ARM_KINEMATICS_LOCAL
#endif
#define ARM_KINEMATICS_PUBLIC_TYPE
#endif

#endif // ARM_KINEMATICS__VISIBILITY_CONTROL_H_
