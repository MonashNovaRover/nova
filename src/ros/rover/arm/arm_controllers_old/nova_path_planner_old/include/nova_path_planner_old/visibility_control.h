#ifndef NOVA_PATH_PLANNER_OLD__VISIBILITY_CONTROL_H_
#define NOVA_PATH_PLANNER_OLD__VISIBILITY_CONTROL_H_

// This logic was borrowed (then namespaced) from the examples on the gcc wiki:
//     https://gcc.gnu.org/wiki/Visibility

#if defined _WIN32 || defined __CYGWIN__
#ifdef __GNUC__
#define NOVA_PATH_PLANNER_OLD_EXPORT __attribute__((dllexport))
#define NOVA_PATH_PLANNER_OLD_IMPORT __attribute__((dllimport))
#else
#define NOVA_PATH_PLANNER_OLD_EXPORT __declspec(dllexport)
#define NOVA_PATH_PLANNER_OLD_IMPORT __declspec(dllimport)
#endif
#ifdef NOVA_PATH_PLANNER_OLD_BUILDING_DLL
#define NOVA_PATH_PLANNER_OLD_PUBLIC NOVA_PATH_PLANNER_OLD_EXPORT
#else
#define NOVA_PATH_PLANNER_OLD_PUBLIC NOVA_PATH_PLANNER_OLD_IMPORT
#endif
#define NOVA_PATH_PLANNER_OLD_PUBLIC_TYPE NOVA_PATH_PLANNER_OLD_PUBLIC
#define NOVA_PATH_PLANNER_OLD_LOCAL
#else
#define NOVA_PATH_PLANNER_OLD_EXPORT __attribute__((visibility("default")))
#define NOVA_PATH_PLANNER_OLD_IMPORT
#if __GNUC__ >= 4
#define NOVA_PATH_PLANNER_OLD_PUBLIC __attribute__((visibility("default")))
#define NOVA_PATH_PLANNER_OLD_LOCAL __attribute__((visibility("hidden")))
#else
#define NOVA_PATH_PLANNER_OLD_PUBLIC
#define NOVA_PATH_PLANNER_OLD_LOCAL
#endif
#define NOVA_PATH_PLANNER_OLD_PUBLIC_TYPE
#endif

#endif // NOVA_PATH_PLANNER_OLD__VISIBILITY_CONTROL_H_
