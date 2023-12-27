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

#include "irrlichttypes.h"
#include <list>
#include <set>
#include <vector>
#include <string>
#include <map>
#include "json-forwards.h"
#include <unordered_set>
#include "util/basic_macros.h"
#include "config.h"
#include "metadata.h"
#include "subgames.h"

class ModStorageDatabase;

#define MODNAME_ALLOWED_CHARS "abcdefghijklmnopqrstuvwxyz0123456789_"

struct ModSpec
{
	String name;
	String author;
	String path;
	String desc;
	int release = 0;

	// if normal mod:
	std::unordered_set<String> depends;
	std::unordered_set<String> optdepends;
	std::unordered_set<String> unsatisfied_depends;

	bool part_of_modpack = false;
	bool is_modpack = false;

	/**
	 * A constructed canonical path to represent this mod's location.
	 * This intended to be used as an identifier for a modpath that tolerates file movement,
	 * and cannot be used to read the mod files.
	 *
	 * Note that `mymod` is the directory name, not the mod name specified in mod.conf.
	 *
	 * Ex:
	 *
	 * - mods/mymod
	 * - mods/mymod (1)
	 *     (^ this would have name=mymod in mod.conf)
	 * - mods/modpack1/mymod
	 * - games/mygame/mods/mymod
	 * - worldmods/mymod
	 */
	String virtual_path;

	// For logging purposes
	std::vector<const char *> deprecation_msgs;

	// if modpack:
	std::map<String, ModSpec> modpack_content;

	ModSpec()
	{
	}

	ModSpec(const String &name, const String &path, bool part_of_modpack, const String &virtual_path) :
			name(name), path(path), part_of_modpack(part_of_modpack), virtual_path(virtual_path)
	{
	}

	void checkAndLog() const;
};

/**
 * Retrieves depends, optdepends, is_modpack and modpack_content
 *
 * @returns false if not a mod
 */
bool parseModContents(ModSpec &mod);

/**
 * Gets a list of all mods and modpacks in path
 *
 * @param Path to search, should be absolute
 * @param part_of_modpack Is this searching within a modpack?
 * @param virtual_path Virtual path for this directory, see comment in ModSpec
 * @returns map of mods
 */
std::map<String, ModSpec> getModsInPath(const String &path,
		const String &virtual_path, bool part_of_modpack = false);

// replaces modpack Modspecs with their content
std::vector<ModSpec> flattenMods(const std::map<String, ModSpec> &mods);


class ModStorage : public IMetadata
{
public:
	ModStorage() = delete;
	ModStorage(const String &mod_name, ModStorageDatabase *database);
	~ModStorage() = default;

	const String &getModName() const { return m_mod_name; }

	void clear() override;

	bool contains(const String &name) const override;

	bool setString(const String &name, const String &var) override;

	const StringMap &getStrings(StringMap *place) const override;

	const std::vector<String> &getKeys(std::vector<String> *place) const override;

protected:
	const String *getStringRaw(const String &name,
			String *place) const override;

private:
	String m_mod_name;
	ModStorageDatabase *m_database;
};
