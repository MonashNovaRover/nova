#ifndef NOVA_TWISTMAPPER__VISIBILITY_CONTROL_H_
#define NOVA_TWISTMAPPER__VISIBILITY_CONTROL_H_

// This logic was borrowed (then namespaced) from the examples on the gcc wiki:
//     https://gcc.gnu.org/wiki/Visibility

#if defined _WIN32 || defined __CYGWIN__
#ifdef __GNUC__
#define NOVA_TWISTMAPPER_EXPORT __attribute__((dllexport))
#define NOVA_TWISTMAPPER_IMPORT __attribute__((dllimport))
#else
#define NOVA_TWISTMAPPER_EXPORT __declspec(dllexport)
#define NOVA_TWISTMAPPER_IMPORT __declspec(dllimport)
#endif
#ifdef NOVA_TWISTMAPPER_BUILDING_DLL
#define NOVA_TWISTMAPPER_PUBLIC NOVA_TWISTMAPPER_EXPORT
#else
#define NOVA_TWISTMAPPER_PUBLIC NOVA_TWISTMAPPER_IMPORT
#endif
#define NOVA_TWISTMAPPER_PUBLIC_TYPE NOVA_TWISTMAPPER_PUBLIC
#define NOVA_TWISTMAPPER_LOCAL
#else
#define NOVA_TWISTMAPPER_EXPORT __attribute__((visibility("default")))
#define NOVA_TWISTMAPPER_IMPORT
#if __GNUC__ >= 4
#define NOVA_TWISTMAPPER_PUBLIC __attribute__((visibility("default")))
#define NOVA_TWISTMAPPER_LOCAL __attribute__((visibility("hidden")))
#else
#define NOVA_TWISTMAPPER_PUBLIC
#define NOVA_TWISTMAPPER_LOCAL
#endif
#define NOVA_TWISTMAPPER_PUBLIC_TYPE
#endif

#endif // NOVA_TWISTMAPPER__VISIBILITY_CONTROL_H_
