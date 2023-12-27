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

#include <string>
#include <set>
#include <unordered_map>
#include <vector>

class Settings;

struct SubgameSpec
{
	String id;
	String title;
	String author;
	int release;
	String path;
	String gamemods_path;

	/**
	 * Map from virtual path to mods path
	 */
	std::unordered_map<String, String> addon_mods_paths;
	String menuicon_path;

	// For logging purposes
	std::vector<const char *> deprecation_msgs;

	SubgameSpec(const String &id = "", const String &path = "",
			const String &gamemods_path = "",
			const std::unordered_map<String, String> &addon_mods_paths = {},
			const String &title = "",
			const String &menuicon_path = "",
			const String &author = "", int release = 0) :
			id(id),
			title(title), author(author), release(release), path(path),
			gamemods_path(gamemods_path), addon_mods_paths(addon_mods_paths),
			menuicon_path(menuicon_path)
	{
	}

	bool isValid() const { return (!id.empty() && !path.empty()); }
	void checkAndLog() const;
};

SubgameSpec findSubgame(const String &id);
SubgameSpec findWorldSubgame(const String &world_path);

std::set<String> getAvailableGameIds();
std::vector<SubgameSpec> getAvailableGames();
// Get the list of paths to mods in the environment variable $MINETEST_MOD_PATH
std::vector<String> getEnvModPaths();

bool getWorldExists(const String &world_path);
//! Try to get the displayed name of a world
String getWorldName(const String &world_path, const String &default_name);
String getWorldGameId(const String &world_path, bool can_be_legacy = false);

struct WorldSpec
{
	String path;
	String name;
	String gameid;

	WorldSpec(const String &path = "", const String &name = "",
			const String &gameid = "") :
			path(path),
			name(name), gameid(gameid)
	{
	}

	bool isValid() const
	{
		return (!name.empty() && !path.empty() && !gameid.empty());
	}
};

std::vector<WorldSpec> getAvailableWorlds();

// loads the subgame's config and creates world directory
// and world.mt if they don't exist
void loadGameConfAndInitWorld(const String &path, const String &name,
		const SubgameSpec &gamespec, bool create_world);
