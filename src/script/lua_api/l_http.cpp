/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "lua_api/l_internal.h"
#include "common/c_converter.h"
#include "common/c_content.h"
#include "lua_api/l_http.h"
#include "cpp_api/s_security.h"
#include "httpfetch.h"
#include "settings.h"
#include "debug.h"
#include "log.h"

#include <iomanip>

#define HTTP_API(name) \
	lua_pushstring(L, #name); \
	lua_pushcfunction(L, l_http_##name); \
	lua_settable(L, -3);

int ModApiHttp::l_set_http_api_lua(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	return 0;
}

void ModApiHttp::Initialize(lua_State *L, int top)
{
	// Define this function anyway so builtin can call it without checking
	API_FCT(set_http_api_lua);
}

void ModApiHttp::InitializeAsync(lua_State *L, int top)
{
}
