mark_as_advanced(GMP_LIBRARY GMP_INCLUDE_DIR)

find_library(GMP_LIBRARY NAMES gmp)
find_path(GMP_INCLUDE_DIR NAMES gmp.h)

if(GMP_LIBRARY AND GMP_INCLUDE_DIR)
	message (STATUS "Using GMP provided by system.")
else()
	message (STATUS "Detecting GMP from system failed.")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GMP DEFAULT_MSG GMP_LIBRARY GMP_INCLUDE_DIR)
