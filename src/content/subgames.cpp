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

#include <common/c_internal.h>
#include "content/subgames.h"
#include "porting.h"
#include "filesys.h"
#include "settings.h"
#include "log.h"
#include "util/strfnd.h"
#include "defaultsettings.h" // for set_default_settings
#include "map_settings_manager.h"
#include "util/string.h"

// The maximum number of identical world names allowed
#define MAX_WORLD_NAMES 100

namespace
{

bool getGameMinetestConfig(const String &game_path, Settings &conf)
{
	String conf_path = game_path + DIR_DELIM + "minetest.conf";
	return conf.readConfigFile(conf_path.c_str());
}

}


void SubgameSpec::checkAndLog() const
{
	// Log deprecation messages
	auto handling_mode = get_deprecated_handling_mode();
	if (!deprecation_msgs.empty() && handling_mode != DeprecatedHandlingMode::Ignore) {
		std::ostringstream os;
		os << "Game " << title << " at " << path << ":" << std::endl;
		for (auto msg : deprecation_msgs)
			os << "\t" << msg << std::endl;

		if (handling_mode == DeprecatedHandlingMode::Error)
			throw ModError(os.str());
		else
			warningstream << os.str();
	}
}


struct GameFindPath
{
	String path;
	bool user_specific;
	GameFindPath(const String &path, bool user_specific) :
			path(path), user_specific(user_specific)
	{
	}
};

String getSubgamePathEnv()
{
	char *subgame_path = getenv("MINETEST_SUBGAME_PATH");
	return subgame_path ? String(subgame_path) : "";
}

SubgameSpec findSubgame(const String &id)
{
	if (id.empty())
		return SubgameSpec();
	String share = porting::path_share;
	String user = porting::path_user;

	// Get games install locations
	Strfnd search_paths(getSubgamePathEnv());

	// Get all possible paths fo game
	std::vector<GameFindPath> find_paths;
	while (!search_paths.at_end()) {
		String path = search_paths.next(PATH_DELIM);
		path.append(DIR_DELIM).append(id);
		find_paths.emplace_back(path, false);
		path.append("_game");
		find_paths.emplace_back(path, false);
	}

	String game_base = DIR_DELIM;
	game_base = game_base.append("games").append(DIR_DELIM).append(id);
	String game_suffixed = game_base + "_game";
	find_paths.emplace_back(user + game_suffixed, true);
	find_paths.emplace_back(user + game_base, true);
	find_paths.emplace_back(share + game_suffixed, false);
	find_paths.emplace_back(share + game_base, false);

	// Find game directory
	String game_path;
	bool user_game = true; // Game is in user's directory
	for (const GameFindPath &find_path : find_paths) {
		const String &try_path = find_path.path;
		if (fs::PathExists(try_path)) {
			game_path = try_path;
			user_game = find_path.user_specific;
			break;
		}
	}

	if (game_path.empty())
		return SubgameSpec();

	String gamemod_path = game_path + DIR_DELIM + "mods";

	// Find mod directories
	std::unordered_map<String, String> mods_paths;
	mods_paths["mods"] = user + DIR_DELIM + "mods";
	if (!user_game && user != share)
		mods_paths["share"] = share + DIR_DELIM + "mods";

	for (const String &mod_path : getEnvModPaths()) {
		mods_paths[fs::AbsolutePath(mod_path)] = mod_path;
	}

	// Get meta
	String conf_path = game_path + DIR_DELIM + "game.conf";
	Settings conf;
	conf.readConfigFile(conf_path.c_str());

	String game_title;
	if (conf.exists("title"))
		game_title = conf.get("title");
	else if (conf.exists("name"))
		game_title = conf.get("name");
	else
		game_title = id;

	String game_author;
	if (conf.exists("author"))
		game_author = conf.get("author");

	int game_release = 0;
	if (conf.exists("release"))
		game_release = conf.getS32("release");

	String menuicon_path;

	SubgameSpec spec(id, game_path, gamemod_path, mods_paths, game_title,
			menuicon_path, game_author, game_release);

	if (conf.exists("name") && !conf.exists("title"))
		spec.deprecation_msgs.push_back("\"name\" setting in game.conf is deprecated, please use \"title\" instead");

	return spec;
}

SubgameSpec findWorldSubgame(const String &world_path)
{
	String world_gameid = getWorldGameId(world_path, true);
	// See if world contains an embedded game; if so, use it.
	String world_gamepath = world_path + DIR_DELIM + "game";
	if (fs::PathExists(world_gamepath)) {
		SubgameSpec gamespec;
		gamespec.id = world_gameid;
		gamespec.path = world_gamepath;
		gamespec.gamemods_path = world_gamepath + DIR_DELIM + "mods";

		Settings conf;
		String conf_path = world_gamepath + DIR_DELIM + "game.conf";
		conf.readConfigFile(conf_path.c_str());

		if (conf.exists("title"))
			gamespec.title = conf.get("title");
		else if (conf.exists("name"))
			gamespec.title = conf.get("name");
		else
			gamespec.title = world_gameid;

		return gamespec;
	}
	return findSubgame(world_gameid);
}

std::set<String> getAvailableGameIds()
{
	std::set<String> gameids;
	std::set<String> gamespaths;
	gamespaths.insert(porting::path_share + DIR_DELIM + "games");
	gamespaths.insert(porting::path_user + DIR_DELIM + "games");

	Strfnd search_paths(getSubgamePathEnv());

	while (!search_paths.at_end())
		gamespaths.insert(search_paths.next(PATH_DELIM));

	for (const String &gamespath : gamespaths) {
		std::vector<fs::DirListNode> dirlist = fs::GetDirListing(gamespath);
		for (const fs::DirListNode &dln : dirlist) {
			if (!dln.dir)
				continue;

			// If configuration file is not found or broken, ignore game
			Settings conf;
			String conf_path = gamespath + DIR_DELIM + dln.name +
						DIR_DELIM + "game.conf";
			if (!conf.readConfigFile(conf_path.c_str()))
				continue;

			// Add it to result
			const char *ends[] = {"_game", NULL};
			String shorter = removeStringEnd(dln.name, ends);
			if (!shorter.empty())
				gameids.insert(shorter);
			else
				gameids.insert(dln.name);
		}
	}
	return gameids;
}

std::vector<SubgameSpec> getAvailableGames()
{
	std::vector<SubgameSpec> specs;
	std::set<String> gameids = getAvailableGameIds();
	specs.reserve(gameids.size());
	for (const auto &gameid : gameids)
		specs.push_back(findSubgame(gameid));
	return specs;
}

#define LEGACY_GAMEID "minetest"

bool getWorldExists(const String &world_path)
{
	return (fs::PathExists(world_path + DIR_DELIM + "map_meta.txt") ||
			fs::PathExists(world_path + DIR_DELIM + "world.mt"));
}

//! Try to get the displayed name of a world
String getWorldName(const String &world_path, const String &default_name)
{
	String conf_path = world_path + DIR_DELIM + "world.mt";
	Settings conf;
	bool succeeded = conf.readConfigFile(conf_path.c_str());
	if (!succeeded) {
		return default_name;
	}

	if (!conf.exists("world_name"))
		return default_name;
	return conf.get("world_name");
}

String getWorldGameId(const String &world_path, bool can_be_legacy)
{
	String conf_path = world_path + DIR_DELIM + "world.mt";
	Settings conf;
	bool succeeded = conf.readConfigFile(conf_path.c_str());
	if (!succeeded) {
		if (can_be_legacy) {
			// If map_meta.txt exists, it is probably an old minetest world
			if (fs::PathExists(world_path + DIR_DELIM + "map_meta.txt"))
				return LEGACY_GAMEID;
		}
		return "";
	}
	if (!conf.exists("gameid"))
		return "";
	// The "mesetint" gameid has been discarded
	if (conf.get("gameid") == "mesetint")
		return "minetest";
	return conf.get("gameid");
}

String getWorldPathEnv()
{
	char *world_path = getenv("MINETEST_WORLD_PATH");
	return world_path ? String(world_path) : "";
}

std::vector<WorldSpec> getAvailableWorlds()
{
	std::vector<WorldSpec> worlds;
	std::set<String> worldspaths;

	Strfnd search_paths(getWorldPathEnv());

	while (!search_paths.at_end())
		worldspaths.insert(search_paths.next(PATH_DELIM));

	worldspaths.insert(porting::path_user + DIR_DELIM + "worlds");
	infostream << "Searching worlds..." << std::endl;
	for (const String &worldspath : worldspaths) {
		infostream << "  In " << worldspath << ": ";
		std::vector<fs::DirListNode> dirvector = fs::GetDirListing(worldspath);
		for (const fs::DirListNode &dln : dirvector) {
			if (!dln.dir)
				continue;
			String fullpath = worldspath + DIR_DELIM + dln.name;
			String name = getWorldName(fullpath, dln.name);
			// Just allow filling in the gameid always for now
			bool can_be_legacy = true;
			String gameid = getWorldGameId(fullpath, can_be_legacy);
			WorldSpec spec(fullpath, name, gameid);
			if (!spec.isValid()) {
				infostream << "(invalid: " << name << ") ";
			} else {
				infostream << name << " ";
				worlds.push_back(spec);
			}
		}
		infostream << std::endl;
	}
	// Check old world location
	do {
		String fullpath = porting::path_user + DIR_DELIM + "world";
		if (!fs::PathExists(fullpath))
			break;
		String name = "Old World";
		String gameid = getWorldGameId(fullpath, true);
		WorldSpec spec(fullpath, name, gameid);
		infostream << "Old world found." << std::endl;
		worlds.push_back(spec);
	} while (false);
	infostream << worlds.size() << " found." << std::endl;
	return worlds;
}

void loadGameConfAndInitWorld(const String &path, const String &name,
		const SubgameSpec &gamespec, bool create_world)
{
	String final_path = path;

	// If we're creating a new world, ensure that the path isn't already taken
	if (create_world) {
		int counter = 1;
		while (fs::PathExists(final_path) && counter < MAX_WORLD_NAMES) {
			final_path = path + "_" + std::to_string(counter);
			counter++;
		}

		if (fs::PathExists(final_path)) {
			throw BaseException("Too many similar filenames");
		}
	}

	Settings *game_settings = Settings::getLayer(SL_GAME);
	const bool new_game_settings = (game_settings == nullptr);
	if (new_game_settings) {
		// Called by main-menu without a Server instance running
		// -> create and free manually
		game_settings = Settings::createLayer(SL_GAME);
	}

	getGameMinetestConfig(gamespec.path, *game_settings);
	game_settings->removeSecureSettings();

	infostream << "Initializing world at " << final_path << std::endl;

	fs::CreateAllDirs(final_path);

	// Create world.mt if does not already exist
	String worldmt_path = final_path + DIR_DELIM "world.mt";
	if (!fs::PathExists(worldmt_path)) {
		Settings gameconf;
		String gameconf_path = gamespec.path + DIR_DELIM "game.conf";
		gameconf.readConfigFile(gameconf_path.c_str());

		Settings conf; // for world.mt

		conf.set("world_name", name);
		conf.set("gameid", gamespec.id);

		String backend = "sqlite3";
		if (gameconf.exists("map_persistent") && !gameconf.getBool("map_persistent")) {
			backend = "dummy";
		}
		conf.set("backend", backend);

		conf.set("player_backend", "sqlite3");
		conf.set("auth_backend", "sqlite3");
		conf.set("mod_storage_backend", "sqlite3");
		conf.setBool("creative_mode", g_settings->getBool("creative_mode"));
		conf.setBool("enable_damage", g_settings->getBool("enable_damage"));
		if (MAP_BLOCKSIZE != 16)
			conf.set("blocksize", std::to_string(MAP_BLOCKSIZE));

		if (!conf.updateConfigFile(worldmt_path.c_str())) {
			throw BaseException("Failed to update the config file");
		}
	}

	// Create map_meta.txt if does not already exist
	String map_meta_path = final_path + DIR_DELIM + "map_meta.txt";
	if (!fs::PathExists(map_meta_path)) {
		MapSettingsManager mgr(map_meta_path);

		mgr.setMapSetting("seed", g_settings->get("fixed_map_seed"));

		mgr.makeMapgenParams();
		mgr.saveMapMeta();
	}

	// The Settings object is no longer needed for created worlds
	if (new_game_settings)
		delete game_settings;
}

std::vector<String> getEnvModPaths()
{
	const char *c_mod_path = getenv("MINETEST_MOD_PATH");
	std::vector<String> paths;
	Strfnd search_paths(c_mod_path ? c_mod_path : "");
	while (!search_paths.at_end())
		paths.push_back(search_paths.next(PATH_DELIM));
	return paths;
}
