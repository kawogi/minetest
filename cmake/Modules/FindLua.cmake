# Look for Lua library to use
# This selects LuaJIT by default

find_package(LuaJIT)
if(LUAJIT_FOUND)
	message (STATUS "Using LuaJIT provided by system.")
elseif(REQUIRE_LUAJIT)
	message(FATAL_ERROR "LuaJIT not found whereas REQUIRE_LUAJIT=\"TRUE\" is used.\n"
		"To continue, either install LuaJIT or do not use REQUIRE_LUAJIT=\"TRUE\".")
endif()
