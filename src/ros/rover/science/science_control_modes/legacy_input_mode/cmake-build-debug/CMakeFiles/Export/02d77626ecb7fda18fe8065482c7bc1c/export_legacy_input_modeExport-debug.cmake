#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "legacy_input_mode::legacy_input_mode" for configuration "Debug"
set_property(TARGET legacy_input_mode::legacy_input_mode APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(legacy_input_mode::legacy_input_mode PROPERTIES
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/liblegacy_input_mode.so"
  IMPORTED_SONAME_DEBUG "liblegacy_input_mode.so"
  )

list(APPEND _cmake_import_check_targets legacy_input_mode::legacy_input_mode )
list(APPEND _cmake_import_check_files_for_legacy_input_mode::legacy_input_mode "${_IMPORT_PREFIX}/lib/liblegacy_input_mode.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
