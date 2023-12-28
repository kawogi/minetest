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

#pragma once

#include <iostream>
#include <exception>
#include <cassert>
#include "gettime.h"
#include "log.h"

#define FUNCTION_NAME __PRETTY_FUNCTION__

// Whether to catch all std::exceptions.
// When "catching", the program will abort with an error message.
// In debug mode, leave these for the debugger and don't catch them.
#define CATCH_UNHANDLED_EXCEPTIONS 0

/* Abort program execution immediately
 */
[[noreturn]] extern void fatal_error_fn(
		const char *msg, const char *file,
		unsigned int line, const char *function);

#define FATAL_ERROR(msg) \
	fatal_error_fn((msg), __FILE__, __LINE__, FUNCTION_NAME)

#define FATAL_ERROR_IF(expr, msg) \
	((expr) \
	? fatal_error_fn((msg), __FILE__, __LINE__, FUNCTION_NAME) \
	: (void)(0))

/*
	sanity_check()
	Equivalent to assert() but persists in Release builds (i.e. when NDEBUG is
	defined)
*/

[[noreturn]] extern void sanity_check_fn(
		const char *assertion, const char *file,
		unsigned int line, const char *function);

#define SANITY_CHECK(expr) \
	((expr) \
	? (void)(0) \
	: sanity_check_fn(#expr, __FILE__, __LINE__, FUNCTION_NAME))

#define sanity_check(expr) SANITY_CHECK(expr)

std::string debug_describe_exc(const std::exception &e);

void debug_set_exception_handler();
