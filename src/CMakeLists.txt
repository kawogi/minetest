project(minetest)

INCLUDE(CheckTypeSize)
INCLUDE(CheckIncludeFiles)
INCLUDE(CheckLibraryExists)

check_type_size(int SIZEOF_INT BUILTIN_TYPES_ONLY LANGUAGE CXX)
if(SIZEOF_INT LESS 4)
	message(FATAL_ERROR "Minetest will not work with int less than 32 bits wide.")
endif()

check_type_size(size_t SIZEOF_SIZE_T LANGUAGE CXX)
if(SIZEOF_SIZE_T LESS 4)
	message(FATAL_ERROR "Minetest will not work with size_t less than 32 bits wide.")
endif()

# Add custom SemiDebug build mode
set(CMAKE_CXX_FLAGS_SEMIDEBUG "-O1 -g -Wall" CACHE STRING
	"Flags used by the C++ compiler during semidebug builds."
	FORCE
)
set(CMAKE_C_FLAGS_SEMIDEBUG "-O1 -g -Wall -pedantic" CACHE STRING
	"Flags used by the C compiler during semidebug builds."
	FORCE
)
mark_as_advanced(
	CMAKE_CXX_FLAGS_SEMIDEBUG
	CMAKE_C_FLAGS_SEMIDEBUG
)
set(CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}" CACHE STRING
	"Choose the type of build. Options are: None Debug SemiDebug RelWithDebInfo MinSizeRel."
	FORCE
)


# Set some random things default to not being visible in the GUI
mark_as_advanced(EXECUTABLE_OUTPUT_PATH LIBRARY_OUTPUT_PATH)

find_package(SQLite3 REQUIRED)

OPTION(ENABLE_SPATIAL "Enable SpatialIndex AreaStore backend" TRUE)
set(USE_SPATIAL FALSE)

if(ENABLE_SPATIAL)
	find_library(SPATIAL_LIBRARY spatialindex)
	find_path(SPATIAL_INCLUDE_DIR spatialindex/SpatialIndex.h)
	if(SPATIAL_LIBRARY AND SPATIAL_INCLUDE_DIR)
		set(USE_SPATIAL TRUE)
		message(STATUS "SpatialIndex AreaStore backend enabled.")
		include_directories(${SPATIAL_INCLUDE_DIR})
	else(SPATIAL_LIBRARY AND SPATIAL_INCLUDE_DIR)
		message(STATUS "SpatialIndex not found!")
	endif(SPATIAL_LIBRARY AND SPATIAL_INCLUDE_DIR)
endif(ENABLE_SPATIAL)


find_package(ZLIB REQUIRED)
find_package(Zstd REQUIRED)


if(NOT MSVC)
	set(USE_GPROF FALSE CACHE BOOL "Use -pg flag for g++")
endif()

# Haiku endian support
if(HAIKU)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_BSD_SOURCE")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_BSD_SOURCE")
endif()

# Use cmake_config.h
add_definitions(-DUSE_CMAKE_CONFIG_H)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
set(PLATFORM_LIBS Threads::Threads)

# Unix probably
set(PLATFORM_LIBS ${PLATFORM_LIBS} ${CMAKE_DL_LIBS})
check_library_exists(rt clock_gettime "" HAVE_LIBRT)
if (HAVE_LIBRT)
	set(PLATFORM_LIBS -lrt ${PLATFORM_LIBS})
endif(HAVE_LIBRT)

find_library(ICONV_LIBRARY iconv)
mark_as_advanced(ICONV_LIBRARY)
if (ICONV_LIBRARY)
	set(PLATFORM_LIBS ${PLATFORM_LIBS} ${ICONV_LIBRARY})
endif()

if (HAIKU)
	set(PLATFORM_LIBS ${PLATFORM_LIBS} network)
endif()

check_include_files(endian.h HAVE_ENDIAN_H)

configure_file(
	"${PROJECT_SOURCE_DIR}/cmake_config.h.in"
	"${PROJECT_BINARY_DIR}/cmake_config.h"
)


# Add a target that always rebuilds cmake_config_githash.h
add_custom_target(GenerateVersion
	COMMAND ${CMAKE_COMMAND}
	-D "GENERATE_VERSION_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
	-D "GENERATE_VERSION_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}"
	-D "VERSION_STRING=${VERSION_STRING}"
	-D "DEVELOPMENT_BUILD=${DEVELOPMENT_BUILD}"
	-P "${CMAKE_SOURCE_DIR}/cmake/Modules/GenerateVersion.cmake"
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")


add_subdirectory(threading)
add_subdirectory(content)
add_subdirectory(database)
add_subdirectory(mapgen)
add_subdirectory(network)
add_subdirectory(script)
add_subdirectory(unittest)
add_subdirectory(util)
add_subdirectory(irrlicht)
add_subdirectory(server)

set(common_SRCS
	${database_SRCS}
	${mapgen_SRCS}
	${server_SRCS}
	${content_SRCS}
	chat.cpp
	clientiface.cpp
	collision.cpp
	content_mapnode.cpp
	content_nodemeta.cpp
	convert_json.cpp
	craftdef.cpp
	debug.cpp
	defaultsettings.cpp
	emerge.cpp
	environment.cpp
	face_position_cache.cpp
	filesys.cpp
	gettext.cpp
	httpfetch.cpp
	hud.cpp
	inventory.cpp
	inventorymanager.cpp
	itemdef.cpp
	itemstackmetadata.cpp
	light.cpp
	lighting.cpp
	log.cpp
	main.cpp
	map.cpp
	map_settings_manager.cpp
	mapblock.cpp
	mapnode.cpp
	mapsector.cpp
	metadata.cpp
	modchannels.cpp
	nameidmapping.cpp
	nodedef.cpp
	nodemetadata.cpp
	nodetimer.cpp
	noise.cpp
	objdef.cpp
	object_properties.cpp
	particles.cpp
	pathfinder.cpp
	player.cpp
	porting.cpp
	profiler.cpp
	raycast.cpp
	reflowscan.cpp
	remoteplayer.cpp
	rollback.cpp
	rollback_interface.cpp
	serialization.cpp
	server.cpp
	serverenvironment.cpp
	serverlist.cpp
	settings.cpp
	staticobject.cpp
	texture_override.cpp
	tileanimation.cpp
	tool.cpp
	translation.cpp
	version.cpp
	voxel.cpp
	voxelalgorithms.cpp
	hud.cpp
	${common_network_SRCS}
	${JTHREAD_SRCS}
	${common_SCRIPT_SRCS}
	${UTIL_SRCS}
)

if(BUILD_UNITTESTS)
	set(common_SRCS ${common_SRCS} ${UNITTEST_SRCS})
endif()

# # Client sources
# set(client_SRCS
# 	${client_SRCS}
# 	${common_SRCS}
# 	${gui_SRCS}
# 	${client_network_SRCS}
# 	${client_irrlicht_SRCS}
# 	${client_SCRIPT_SRCS}
# )

# if(BUILD_UNITTESTS)
# 	set(client_SRCS ${client_SRCS} ${UNITTEST_CLIENT_SRCS})
# endif()

# list(SORT client_SRCS)

# Server sources
set(server_SRCS
	${common_SRCS}
)
list(SORT server_SRCS)

# Avoid source_group on broken CMake version.
# see issue #7074 #7075
if (CMAKE_VERSION VERSION_GREATER 3.8.1)
	source_group(TREE ${PROJECT_SOURCE_DIR} PREFIX "Source Files" FILES ${client_SRCS})
	source_group(TREE ${PROJECT_SOURCE_DIR} PREFIX "Source Files" FILES ${server_SRCS})
endif()

include_directories(
	${PROJECT_BINARY_DIR}
	${PROJECT_SOURCE_DIR}
	${PROJECT_SOURCE_DIR}/script
)
include_directories(SYSTEM
	${ZLIB_INCLUDE_DIR}
	${ZSTD_INCLUDE_DIR}
	${SQLITE3_INCLUDE_DIR}
	${LUA_INCLUDE_DIR}
	${GMP_INCLUDE_DIR}
	${JSON_INCLUDE_DIR}
	${LUA_BIT_INCLUDE_DIR}
)

# When cross-compiling assume the user doesn't want to run the executable anyway,
# otherwise place it in <source dir>/bin/ since Minetest can only run from there.
if(NOT CMAKE_CROSSCOMPILING)
	set(EXECUTABLE_OUTPUT_PATH "${CMAKE_SOURCE_DIR}/bin")
endif()

add_executable(${PROJECT_NAME}server ${server_SRCS} ${extra_windows_SRCS})
add_dependencies(${PROJECT_NAME}server GenerateVersion)

# Doesn't work without PRIVATE/PUBLIC/INTERFACE mode specified.
target_include_directories(${PROJECT_NAME}server PRIVATE ${IRRLICHT_INCLUDES})
target_link_libraries(
	${PROJECT_NAME}server
	${ZLIB_LIBRARIES}
	${ZSTD_LIBRARY}
	${SQLITE3_LIBRARY}
	${JSON_LIBRARY}
	${LUA_LIBRARY}
	${LUA_BIT_LIBRARY}
	${GMP_LIBRARY}
	${PLATFORM_LIBS}
)
set_target_properties(${PROJECT_NAME}server PROPERTIES
	COMPILE_DEFINITIONS "SERVER")

if (USE_SPATIAL)
	target_link_libraries(${PROJECT_NAME}server ${SPATIAL_LIBRARY})
endif()

# See issue #4638
FILE(READ "${CMAKE_SOURCE_DIR}/src/unsupported_language_list.txt" GETTEXT_BLACKLISTED_LOCALES)
STRING(REGEX REPLACE "\n" ";" GETTEXT_BLACKLISTED_LOCALES "${GETTEXT_BLACKLISTED_LOCALES}")

option(APPLY_LOCALE_BLACKLIST "Use a blacklist to avoid known broken locales" TRUE)

if (GETTEXTLIB_FOUND AND APPLY_LOCALE_BLACKLIST)
	set(GETTEXT_USED_LOCALES "")
	foreach(LOCALE ${GETTEXT_AVAILABLE_LOCALES})
		if (NOT "${LOCALE}" IN_LIST GETTEXT_BLACKLISTED_LOCALES)
			list(APPEND GETTEXT_USED_LOCALES ${LOCALE})
		endif()
	endforeach()
	message(STATUS "Locale blacklist applied; Locales used: ${GETTEXT_USED_LOCALES}")
elseif (GETTEXTLIB_FOUND)
	set(GETTEXT_USED_LOCALES ${GETTEXT_AVAILABLE_LOCALES})
endif()

# Set some optimizations and tweaks

include(CheckCSourceCompiles)
include(CheckSymbolExists)

set(CMAKE_REQUIRED_INCLUDES ${LUA_INCLUDE_DIR})
set(CMAKE_REQUIRED_LIBRARIES ${LUA_LIBRARY})
# LuaJIT provides exactly zero ways to determine how recent it is (the version
# is unchanged since 2017), however it happens that string buffers were added
# after the changes which we care about so that works as an indicator.
# (https://github.com/LuaJIT/LuaJIT/commit/4c6b669 March 2021)
unset(HAVE_RECENT_LJ CACHE)
check_symbol_exists(luaopen_string_buffer "lualib.h" HAVE_RECENT_LJ)
if(NOT HAVE_RECENT_LJ)
	string(CONCAT explanation_msg
		"You are using a relatively old version of LuaJIT. We recommend "
		"running a recent version (from git) as older ones are known not "
		"to build/work correctly in all cases.\n"
		"THIS APPLIES ESPECIALLY ON macOS OR Linux/aarch64!")
	message(WARNING ${explanation_msg})
endif()

# GCC or compatible compilers such as Clang
set(WARNING_FLAGS "-Wall -Wextra")
set(WARNING_FLAGS "${WARNING_FLAGS} -Wno-unused-parameter")
if(WARN_ALL)
	set(RELEASE_WARNING_FLAGS "${WARNING_FLAGS}")
else()
	set(RELEASE_WARNING_FLAGS "")
endif()

# Use a safe subset of flags to speed up math calculations:
# - we don't need errno or math exceptions
# - we don't deal with signed zero
set(MATH_FLAGS "-fno-math-errno -fno-trapping-math -fno-signed-zeros")

# Enable SSE for floating point math on 32-bit x86 by default
# reasoning see minetest issue #11810 and https://gcc.gnu.org/wiki/FloatingPointMath
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
	check_c_source_compiles("#ifndef __i686__\n#error\n#endif\nint main(){}" IS_I686)
	if(IS_I686)
		message(STATUS "Detected Intel x86: using SSE instead of x87 FPU")
		set(OTHER_FLAGS "${OTHER_FLAGS} -mfpmath=sse -msse")
	endif()
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG ${RELEASE_WARNING_FLAGS} ${OTHER_FLAGS} -pipe -funroll-loops -O3 -fomit-frame-pointer")
if(CMAKE_SYSTEM_NAME STREQUAL "Linux"
		AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
		AND CMAKE_CXX_COMPILER_VERSION MATCHES "^9\\.")
	# Clang 9 has broken -ffast-math on glibc
else()
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${MATH_FLAGS}")
endif()
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE} -g")
set(CMAKE_CXX_FLAGS_SEMIDEBUG "-g -O1 ${WARNING_FLAGS} ${OTHER_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 ${WARNING_FLAGS} ${OTHER_FLAGS}")

if(UNIX)
	# enable assertions for libstdc++ or libc++
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wp,-D_GLIBCXX_ASSERTIONS -Wp,-D_LIBCPP_ENABLE_ASSERTIONS=1")
endif()
if(USE_GPROF)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pg")
endif()

# Installation

install(TARGETS ${PROJECT_NAME}server DESTINATION ${BINDIR})
