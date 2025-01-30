#ifndef NOVA_IK_CONTROLLER__VISIBILITY_CONTROL_H_
#define NOVA_IK_CONTROLLER__VISIBILITY_CONTROL_H_

// This logic was borrowed (then namespaced) from the examples on the gcc wiki:
//     https://gcc.gnu.org/wiki/Visibility

#if defined _WIN32 || defined __CYGWIN__
#ifdef __GNUC__
#define NOVA_IK_CONTROLLER_EXPORT __attribute__((dllexport))
#define NOVA_IK_CONTROLLER_IMPORT __attribute__((dllimport))
#else
#define NOVA_IK_CONTROLLER_EXPORT __declspec(dllexport)
#define NOVA_IK_CONTROLLER_IMPORT __declspec(dllimport)
#endif
#ifdef NOVA_IK_CONTROLLER_BUILDING_DLL
#define NOVA_IK_CONTROLLER_PUBLIC NOVA_IK_CONTROLLER_EXPORT
#else
#define NOVA_IK_CONTROLLER_PUBLIC NOVA_IK_CONTROLLER_IMPORT
#endif
#define NOVA_IK_CONTROLLER_PUBLIC_TYPE NOVA_IK_CONTROLLER_PUBLIC
#define NOVA_IK_CONTROLLER_LOCAL
#else
#define NOVA_IK_CONTROLLER_EXPORT __attribute__((visibility("default")))
#define NOVA_IK_CONTROLLER_IMPORT
#if __GNUC__ >= 4
#define NOVA_IK_CONTROLLER_PUBLIC __attribute__((visibility("default")))
#define NOVA_IK_CONTROLLER_LOCAL __attribute__((visibility("hidden")))
#else
#define NOVA_IK_CONTROLLER_PUBLIC
#define NOVA_IK_CONTROLLER_LOCAL
#endif
#define NOVA_IK_CONTROLLER_PUBLIC_TYPE
#endif

#endif // NOVA_IK_CONTROLLER__VISIBILITY_CONTROL_H_
