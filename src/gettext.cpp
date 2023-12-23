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
#if USE_GETTEXT
	// First, try to set user override environment
	if (!configured_language.empty()) {
		// Add user specified locale to environment
		setenv("LANGUAGE", configured_language.c_str(), 1);

		// Reload locale with changed environment
		setlocale(LC_ALL, "");
	}
	else {
		/* set current system default locale */
		setlocale(LC_ALL, "");
	}

	std::string name = lowercase(PROJECT_NAME);
	infostream << "Gettext: domainname=\"" << name
		<< "\" path=\"" << path << "\"" << std::endl;

	bindtextdomain(name.c_str(), path);
	textdomain(name.c_str());
#else
	/* set current system default locale */
	setlocale(LC_ALL, "");
#endif // if USE_GETTEXT

	/* no matter what locale is used we need number format to be "C" */
	/* to ensure formspec parameters are evaluated correct!          */

	setlocale(LC_NUMERIC, "C");
	infostream << "Message locale is now set to: "
			<< setlocale(LC_ALL, 0) << std::endl;
}
