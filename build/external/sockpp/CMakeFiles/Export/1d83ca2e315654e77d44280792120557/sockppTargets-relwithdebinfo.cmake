#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Sockpp::sockpp-shared" for configuration "RelWithDebInfo"
set_property(TARGET Sockpp::sockpp-shared APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(Sockpp::sockpp-shared PROPERTIES
  IMPORTED_IMPLIB_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/sockpp.lib"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/bin/sockpp.dll"
  )

list(APPEND _cmake_import_check_targets Sockpp::sockpp-shared )
list(APPEND _cmake_import_check_files_for_Sockpp::sockpp-shared "${_IMPORT_PREFIX}/lib/sockpp.lib" "${_IMPORT_PREFIX}/bin/sockpp.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
