#ifndef NOVA_PATH_PLANNER__VISIBILITY_CONTROL_H_
#define NOVA_PATH_PLANNER__VISIBILITY_CONTROL_H_

// This logic was borrowed (then namespaced) from the examples on the gcc wiki:
//     https://gcc.gnu.org/wiki/Visibility

#if defined _WIN32 || defined __CYGWIN__
#ifdef __GNUC__
#define NOVA_PATH_PLANNER_EXPORT __attribute__((dllexport))
#define NOVA_PATH_PLANNER_IMPORT __attribute__((dllimport))
#else
#define NOVA_PATH_PLANNER_EXPORT __declspec(dllexport)
#define NOVA_PATH_PLANNER_IMPORT __declspec(dllimport)
#endif
#ifdef NOVA_PATH_PLANNER_BUILDING_DLL
#define NOVA_PATH_PLANNER_PUBLIC NOVA_PATH_PLANNER_EXPORT
#else
#define NOVA_PATH_PLANNER_PUBLIC NOVA_PATH_PLANNER_IMPORT
#endif
#define NOVA_PATH_PLANNER_PUBLIC_TYPE NOVA_PATH_PLANNER_PUBLIC
#define NOVA_PATH_PLANNER_LOCAL
#else
#define NOVA_PATH_PLANNER_EXPORT __attribute__((visibility("default")))
#define NOVA_PATH_PLANNER_IMPORT
#if __GNUC__ >= 4
#define NOVA_PATH_PLANNER_PUBLIC __attribute__((visibility("default")))
#define NOVA_PATH_PLANNER_LOCAL __attribute__((visibility("hidden")))
#else
#define NOVA_PATH_PLANNER_PUBLIC
#define NOVA_PATH_PLANNER_LOCAL
#endif
#define NOVA_PATH_PLANNER_PUBLIC_TYPE
#endif

#endif // NOVA_PATH_PLANNER__VISIBILITY_CONTROL_H_
