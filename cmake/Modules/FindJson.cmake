# Look for JsonCpp, with fallback to bundeled version

mark_as_advanced(JSON_LIBRARY JSON_INCLUDE_DIR)
option(ENABLE_SYSTEM_JSONCPP "Enable using a system-wide JsonCpp" TRUE)

find_library(JSON_LIBRARY NAMES jsoncpp)
find_path(JSON_INCLUDE_DIR json/allocator.h PATH_SUFFIXES jsoncpp)

if(JSON_LIBRARY AND JSON_INCLUDE_DIR)
	message(STATUS "Using JsonCpp provided by system.")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Json DEFAULT_MSG JSON_LIBRARY JSON_INCLUDE_DIR)
