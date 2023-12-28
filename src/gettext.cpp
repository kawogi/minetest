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

#include <string>
#include <cstring>
#include <iostream>
#include <cstdlib>
#include "gettext.h"
#include "util/string.h"
#include "log.h"

/******************************************************************************/
void init_gettext(const char *path, const std::string &configured_language,
	int argc, char *argv[])
{
	/* set current system default locale */
	setlocale(LC_ALL, "");

	/* no matter what locale is used we need number format to be "C" */
	/* to ensure formspec parameters are evaluated correct!          */

	setlocale(LC_NUMERIC, "C");
	infostream << "Message locale is now set to: "
			<< setlocale(LC_ALL, 0) << std::endl;
}
