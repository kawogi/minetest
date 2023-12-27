#![warn(clippy::pedantic)]

/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

// #include "irrlichttypes.h" // must be included before anything irrlicht, see comment in the file
// //#include "irrlicht.h" // createDevice
// #include "irrlichttypes_extrabloated.h"
// #include "irrlicht_changes/printing.h"
// #include "chat_interface.h"
// #include "debug.h"
// #include "unittest/test.h"
// #include "server.h"
// #include "filesys.h"
// #include "version.h"
// #include "defaultsettings.h"
// #include "gettext.h"
// #include "log.h"
// #include "util/quicktune.h"
// #include "httpfetch.h"
// #include "gameparams.h"
// #include "database/database.h"
// #include "config.h"
// #include "player.h"
// #include "porting.h"
// #include "network/socket.h"
// #include "mapblock.h"
// #if USE_CURSES
// 	#include "terminal_chat_console.h"
// #endif

// // for version information only
// extern "C" {
// #if USE_LUAJIT
// 	#include <luajit.h>
// #else
// 	#include <lua.h>
// #endif
// }

// #if !defined(__cpp_rtti) || !defined(__cpp_exceptions)
// #error Minetest cannot be built without exceptions or RTTI
// #endif

use std::collections::{HashMap, HashSet};

// #define DEBUGFILE "debug.txt"
pub const DEBUGFILE: &str = "debug.txt";
// #define DEFAULT_SERVER_PORT 30000
pub const DEFAULT_SERVER_PORT: u16 = 30000;

// #define ENV_MT_LOGCOLOR "MT_LOGCOLOR"
pub const ENV_MT_LOGCOLOR: &str = "MT_LOGCOLOR";
// #define ENV_NO_COLOR "NO_COLOR"
pub const ENV_NO_COLOR: &str = "NO_COLOR";
// #define ENV_CLICOLOR "CLICOLOR"
pub const ENV_CLICOLOR: &str = "CLICOLOR";
// #define ENV_CLICOLOR_FORCE "CLICOLOR_FORCE"
pub const ENV_CLICOLOR_FORCE: &str = "CLICOLOR_FORCE";

// typedef std::map<String, ValueSpec> OptionList;
pub type OptionList = HashMap<String, ValueSpec>;

/**********************************************************************
 * Private functions
 **********************************************************************/

// static void get_env_opts(Settings &args);
// static bool get_cmdline_opts(int argc, char *argv[], Settings *cmd_args);
// static void set_allowed_options(OptionList *allowed_options);

// static void print_help(const OptionList &allowed_options);
// static void print_allowed_options(const OptionList &allowed_options);
// static void print_version();
// static void print_worldspecs(const std::vector<WorldSpec> &worldspecs,
// 	std::ostream &os, bool print_name = true, bool print_path = true);
// static void print_modified_quicktune_values();

// static void list_game_ids();
// static void list_worlds(bool print_name, bool print_path);
// static bool setup_log_params(const Settings &cmd_args);
// static bool create_userdata_path();
// static bool use_debugger(int argc, char *argv[]);
// static bool init_common(const Settings &cmd_args, int argc, char *argv[]);
// static void uninit_common();
// static void startup_message();
// static bool read_config_file(const Settings &cmd_args);
// static void init_log_streams(const Settings &cmd_args);

// static bool game_configure(GameParams *game_params, const Settings &cmd_args);
// static void game_configure_port(GameParams *game_params, const Settings &cmd_args);

// static bool game_configure_world(GameParams *game_params, const Settings &cmd_args);
// static bool get_world_from_cmdline(GameParams *game_params, const Settings &cmd_args);
// static bool get_world_from_config(GameParams *game_params, const Settings &cmd_args);
// static bool auto_select_world(GameParams *game_params);
// static String get_clean_world_path(const String &path);

// static bool game_configure_subgame(GameParams *game_params, const Settings &cmd_args);
// static bool get_game_from_cmdline(GameParams *game_params, const Settings &cmd_args);
// static bool determine_subgame(GameParams *game_params);

// static bool run_dedicated_server(const GameParams &game_params, const Settings &cmd_args);
// static bool migrate_map_database(const GameParams &game_params, const Settings &cmd_args);
// static bool recompress_map_database(const GameParams &game_params, const Settings &cmd_args);

/**********************************************************************/


// TODO replace by proper logger
struct FileLogOutput {
}

static file_log_output: FileLogOutput;
static allowed_options: OptionList;

fn main(args: Vec<String>) -> u32
{
	let retval: i32;
	debug_set_exception_handler();

	g_logger.registerThread("Main");
	g_logger.addOutputMaxLevel(&stderr_output, LL_ACTION);

	let cmd_args: Settings;
	get_env_opts(cmd_args);
	let cmd_args_ok: bool = get_cmdline_opts(argc, argv, &cmd_args);
	if (!cmd_args_ok
			|| cmd_args.getFlag("help")
			|| cmd_args.exists("nonopt1")) {
		porting::attachOrCreateConsole();
		print_help(allowed_options);
		return if cmd_args_ok {0} else {1};
	}
	if (cmd_args.getFlag("console")) {
		porting::attachOrCreateConsole();
    }

	if (cmd_args.getFlag("version")) {
		porting::attachOrCreateConsole();
		print_version();
		return 0;
	}

	if (!setup_log_params(cmd_args)) {
		return 1;
    }

	if (cmd_args.getFlag("debugger")) {
		if (!use_debugger(argc, argv)) {
			warningstream << "Continuing without debugger" << std::endl;
        }
	}

	porting::signal_handler_init();
	porting::initializePaths();

	if (!create_userdata_path()) {
		errorstream << "Cannot create user data directory" << std::endl;
		return 1;
	}

	// List gameids if requested
	if (cmd_args.exists("gameid") && cmd_args.get("gameid") == "list") {
		list_game_ids();
		return 0;
	}

	// List worlds, world names, and world paths if requested
	if (cmd_args.exists("worldlist")) {
		if (cmd_args.get("worldlist") == "name") {
			list_worlds(true, false);
		} else if (cmd_args.get("worldlist") == "path") {
			list_worlds(false, true);
		} else if (cmd_args.get("worldlist") == "both") {
			list_worlds(true, true);
		} else {
			errorstream << "Invalid --worldlist value: "
				<< cmd_args.get("worldlist") << std::endl;
			return 1;
		}
		return 0;
	}

	if (!init_common(cmd_args, argc, argv)) {
		return 1;
    }

	if (g_settings.getBool("enable_console")) {
		porting::attachOrCreateConsole();
    }

	let game_params: GameStartData;
	porting::attachOrCreateConsole();
	game_params.is_dedicated_server = true;

	if (!game_configure(&game_params, cmd_args)) {
		return 1;
    }

	sanity_check(!game_params.world_path.empty());

	if (game_params.is_dedicated_server) {
		return if run_dedicated_server(game_params, cmd_args) { 0} else { 1};
    }

	retval = 0;

	// Update configuration file
	if (!g_settings_path.empty()) {
		g_settings.updateConfigFile(g_settings_path.c_str());
    }

	print_modified_quicktune_values();

	return retval;
}


/*****************************************************************************
 * Startup / Init
 *****************************************************************************/


fn get_env_opts(args: &mut Settings)
{
	let mt_logcolor: u8 = std::getenv(ENV_MT_LOGCOLOR);
	if (mt_logcolor) {
		args.set("color", mt_logcolor);
	}

	// CLICOLOR is a de-facto standard option for colors <https://bixense.com/clicolors/>
	// CLICOLOR != 0: ANSI colors are supported (auto-detection, this is the default)
	// CLICOLOR == 0: ANSI colors are NOT supported
	let clicolor: u8 = std::getenv(ENV_CLICOLOR);
	if (clicolor && String(clicolor) == "0") {
		args.set("color", "never");
	}
	// NO_COLOR only specifies that no color is allowed.
	// Implemented according to <http://no-color.org/>
	let no_color: u8 = std::getenv(ENV_NO_COLOR);
	if (no_color && no_color[0]) {
		args.set("color", "never");
	}
	// CLICOLOR_FORCE is another option, which should turn on colors "no matter what".
	let clicolor_force: u8 = std::getenv(ENV_CLICOLOR_FORCE);
	if (clicolor_force && String(clicolor_force) != "0") {
		// should ALWAYS have colors, so we ignore tty (no "auto")
		args.set("color", "always");
	}
}

fn get_cmdline_opts(args: Vec<String>, cmd_args: Settings) -> bool 
{
	set_allowed_options(&allowed_options);

	return cmd_args.parseCommandLine(argc, argv, allowed_options);
}

fn set_allowed_options(allowed_options: OptionList)
{
	allowed_options.clear();

	allowed_options.insert(std::make_pair("help", ValueSpec(VALUETYPE_FLAG,
			_("Show allowed options"))));
	allowed_options.insert(std::make_pair("version", ValueSpec(VALUETYPE_FLAG,
			_("Show version information"))));
	allowed_options.insert(std::make_pair("config", ValueSpec(VALUETYPE_STRING,
			_("Load configuration from specified file"))));
	allowed_options.insert(std::make_pair("port", ValueSpec(VALUETYPE_STRING,
			_("Set network port (UDP)"))));
	allowed_options.insert(std::make_pair("run-unittests", ValueSpec(VALUETYPE_FLAG,
			_("Run the unit tests and exit"))));
	allowed_options.insert(std::make_pair("test-module", ValueSpec(VALUETYPE_STRING,
			_("Only run the specified test module"))));
	allowed_options.insert(std::make_pair("map-dir", ValueSpec(VALUETYPE_STRING,
			_("Same as --world (deprecated)"))));
	allowed_options.insert(std::make_pair("world", ValueSpec(VALUETYPE_STRING,
			_("Set world path (implies local game if used with option --go)"))));
	allowed_options.insert(std::make_pair("worldname", ValueSpec(VALUETYPE_STRING,
			_("Set world by name (implies local game if used with option --go)"))));
	allowed_options.insert(std::make_pair("worldlist", ValueSpec(VALUETYPE_STRING,
			_("Get list of worlds ('path' lists paths, "
			"'name' lists names, 'both' lists both)"))));
	allowed_options.insert(std::make_pair("quiet", ValueSpec(VALUETYPE_FLAG,
			_("Print to console errors only"))));
	allowed_options.insert(std::make_pair("color", ValueSpec(VALUETYPE_STRING,
			_("Coloured logs ('always', 'never' or 'auto'), defaults to 'auto'"
			))));
	allowed_options.insert(std::make_pair("info", ValueSpec(VALUETYPE_FLAG,
			_("Print more information to console"))));
	allowed_options.insert(std::make_pair("verbose",  ValueSpec(VALUETYPE_FLAG,
			_("Print even more information to console"))));
	allowed_options.insert(std::make_pair("trace", ValueSpec(VALUETYPE_FLAG,
			_("Print enormous amounts of information to log and console"))));
	allowed_options.insert(std::make_pair("debugger", ValueSpec(VALUETYPE_FLAG,
			_("Try to automatically attach a debugger before starting (convenience option)"))));
	allowed_options.insert(std::make_pair("logfile", ValueSpec(VALUETYPE_STRING,
			_("Set logfile path ('' = no logging)"))));
	allowed_options.insert(std::make_pair("gameid", ValueSpec(VALUETYPE_STRING,
			_("Set gameid (\"--gameid list\" prints available ones)"))));
	allowed_options.insert(std::make_pair("migrate", ValueSpec(VALUETYPE_STRING,
			_("Migrate from current map backend to another (Only works when using minetestserver or with --server)"))));
	allowed_options.insert(std::make_pair("migrate-players", ValueSpec(VALUETYPE_STRING,
		_("Migrate from current players backend to another (Only works when using minetestserver or with --server)"))));
	allowed_options.insert(std::make_pair("migrate-auth", ValueSpec(VALUETYPE_STRING,
		_("Migrate from current auth backend to another (Only works when using minetestserver or with --server)"))));
	allowed_options.insert(std::make_pair("migrate-mod-storage", ValueSpec(VALUETYPE_STRING,
		_("Migrate from current mod storage backend to another (Only works when using minetestserver or with --server)"))));
	allowed_options.insert(std::make_pair("terminal", ValueSpec(VALUETYPE_FLAG,
			_("Feature an interactive terminal (Only works when using minetestserver or with --server)"))));
	allowed_options.insert(std::make_pair("recompress", ValueSpec(VALUETYPE_FLAG,
			_("Recompress the blocks of the given map database."))));
}

fn print_help(allowed_options: &OptionList)
{
	println!("Allowed options:");
	print_allowed_options(allowed_options);
}

fn print_allowed_options(allowed_options: &OptionList)
{
	for allowed_option in allowed_options {
        let opt: String = "  --" + allowed_option.first;
		if (allowed_option.second.r#type != VALUETYPE_FLAG) {
			opt += _(" <value>");
        }

		let opt_padded: String = padStringRight(opt, 30);
		std::cout << opt_padded;
		if (opt == opt_padded) { // Line is too long to pad
			println!();
            print!("{}", padStringRight("", 30));
        }

		if (allowed_option.second.help) {
			println!("{}", allowed_option.second.help);
        }

		println!();
	}
}

fn print_version()
{
	println!("{} {} ({})", PROJECT_NAME_C, g_version_hash, porting::getPlatformName());
	println!("{}", g_build_info);
}

fn list_game_ids()
{
	let gameids: HashSet<String> = getAvailableGameIds();
	for gameid in gameids {
		println!("{}", gameid);
    }
}

fn list_worlds(print_name: bool, print_path: bool)
{
	println!("Available worlds:");
	let worldspecs: Vec<WorldSpec> = getAvailableWorlds();
	print_worldspecs(&worldspecs, std::cout, print_name, print_path);
}

fn print_worldspecs(worldspecs: &Vec<WorldSpec>,
	os: &mut std::ostream, print_name: bool, print_path: bool)
{
	for worldspec in worldspecs {
		let name: String = worldspec.name;
		let path: String = worldspec.path;
		if (print_name && print_path) {
			os << "\t" << name << "\t\t" << path << std::endl;
		} else if (print_name) {
			os << "\t" << name << std::endl;
		} else if (print_path) {
			os << "\t" << path << std::endl;
		}
	}
}

fn print_modified_quicktune_values()
{
	let header_printed: bool = false;
	let names: Vec<String> = getQuicktuneNames();

	for name in names {
		let val: QuicktuneValue = getQuicktuneValue(name);
		if (!val.modified) {
			continue;
        }
		if (!header_printed) {
			dstream << "Modified quicktune values:" << std::endl;
			header_printed = true;
		}
		dstream << name << " = " << val.getString() << std::endl;
	}
}

fn setup_log_params(cmd_args: &Settings) -> bool
{
	// Quiet mode, print errors only
	if (cmd_args.getFlag("quiet")) {
		g_logger.removeOutput(&stderr_output);
		g_logger.addOutputMaxLevel(&stderr_output, LL_ERROR);
	}

	// Coloured log messages (see log.h)
	let color_mode: String;
	if (cmd_args.exists("color")) {
		color_mode = cmd_args.get("color");
	}
	if (!color_mode.empty()) {
		if (color_mode == "auto") {
			Logger::color_mode = LOG_COLOR_AUTO;
		} else if (color_mode == "always") {
			Logger::color_mode = LOG_COLOR_ALWAYS;
		} else if (color_mode == "never") {
			Logger::color_mode = LOG_COLOR_NEVER;
		} else {
			errorstream << "Invalid color mode: " << color_mode << std::endl;
			return false;
		}
	}

	// In certain cases, output info level on stderr
	if (cmd_args.getFlag("info") || cmd_args.getFlag("verbose") ||
			cmd_args.getFlag("trace") || cmd_args.getFlag("speedtests")) {
		g_logger.addOutput(&stderr_output, LL_INFO);
    }

	// In certain cases, output verbose level on stderr
	if (cmd_args.getFlag("verbose") || cmd_args.getFlag("trace")) {
		g_logger.addOutput(&stderr_output, LL_VERBOSE);
    }

	if (cmd_args.getFlag("trace")) {
		dstream << _("Enabling trace level debug output") << std::endl;
		g_logger.addOutput(&stderr_output, LL_TRACE);
		socket_enable_debug_output = true;
	}

	return true;
}

fn create_userdata_path() -> bool
{
	let success: bool;

	// Create user data directory
	success = fs::CreateAllDirs(porting::path_user);
	return success;
}

// namespace {
	#[allow(unused)]
    fn findProgram(name: &str) -> String {
		let path_c: &str = getenv("PATH");
		if (path_c.is_empty()) {
			return "".to_owned();
        }
		let path_c: std::istringstream = std::istringstream::new(path_c);
		let checkpath: String;
		while (!iss.eof()) {
			std::getline(iss, checkpath, PATH_DELIM[0]);
			if (!checkpath.empty() && checkpath.back() != DIR_DELIM_CHAR) {
				checkpath.push_back(DIR_DELIM_CHAR);
            }
			checkpath.append(name);
			if (fs::IsExecutable(checkpath)) {
				return checkpath;
            }
		}
		return "".to_owned();
	}

    #[allow(unused)]
	const debuggerNames: [&str; 2] = ["gdb", "lldb"];

	fn getDebuggerArgs<T>(out: &T, i: int) {
		if (i == 0) {
			for s in ["-q", "--batch", "-iex", "set confirm off",
				"-ex", "run", "-ex", "bt", "--args"] {
				out.push_back(s);
                }
		} else if (i == 1) {
			for s in ["-Q", "-b", "-o", "run", "-k", "bt\nq", "--"] {
				out.push_back(s);
            }
		}
	}
// }

fn use_debugger(args: &Vec<String>) -> bool
{
	let exec_path: [u8; 1024];
	if (!porting::getCurrentExecPath(exec_path, sizeof(exec_path))) {
		return false;
    }

	let debugger: int = -1;
	let debugger_path: String;
	for i in 0..ARRLEN(debuggerNames) {
		debugger_path = findProgram(debuggerNames[i]);
		if (!debugger_path.empty()) {
			debugger = i;
			break;
		}
	}
	if (debugger == -1) {
		warningstream << "Couldn't find a debugger to use. Try installing gdb or lldb." << std::endl;
		return false;
	}

	let new_args: std::vector<&str>;
	new_args.push_back(debugger_path.c_str());
	getDebuggerArgs(new_args, debugger);
	// Copy the existing arguments
	new_args.push_back(exec_path);
	for i in 1..args.len() {
		if (!strcmp(args[i], "--debugger")) {
			continue;
        }
		new_args.push_back(argv[i]);
	}
	new_args.push_back(nullptr);

	errno = 0;
	execv(new_args[0], const_cast::<&[&[char]]>(new_args.data()));
	warningstream << "execv: " << strerror(errno) << std::endl;
	return false;
}

fn init_common(cmd_args: &Settings, args: &Vec<String>) -> bool
{
	startup_message();
	set_default_settings();

	sockets_init();

	// Initialize g_settings
	Settings::createLayer(SL_GLOBAL);

	// Set cleanup callback(s) to run at process exit
	atexit(uninit_common);

	if (!read_config_file(cmd_args)) {
		return false;
    }

	init_log_streams(cmd_args);

	// Initialize random seed
	srand(time(0));
	mysrand(time(0));

	// Initialize HTTP fetcher
	httpfetch_init(g_settings.getS32("curl_parallel_limit"));

	init_gettext(porting::path_locale.c_str(),
		g_settings.get("language"), argc, argv);

	return true;
}

fn uninit_common()
{
	httpfetch_cleanup();

	sockets_cleanup();

	// It'd actually be okay to leak these but we want to please valgrind...
	for i in 0..SL_TOTAL_COUNT {
		// delete Settings::getLayer((SettingsLayer)i);
    }
}

fn startup_message()
{
	infostream << PROJECT_NAME_C << " " << g_version_hash
		<< "\nwith SER_FMT_VER_HIGHEST_READ="
		<< (int)SER_FMT_VER_HIGHEST_READ << ", "
		<< g_build_info << std::endl;
}

read_config_file(const Settings &cmd_args) -> bool
{
	// Path of configuration file in use
	sanity_check(g_settings_path.empty());	// Sanity check

	if (cmd_args.exists("config")) {
		bool r = g_settings.readConfigFile(cmd_args.get("config").c_str());
		if (!r) {
			errorstream << "Could not read configuration from \""
			            << cmd_args.get("config") << "\"" << std::endl;
			return false;
		}
		g_settings_path = cmd_args.get("config");
	} else {
		std::vector<String> filenames;
		filenames.push_back(porting::path_user + DIR_DELIM + "minetest.conf");
		// Legacy configuration file location
		filenames.push_back(porting::path_user +
				DIR_DELIM + ".." + DIR_DELIM + "minetest.conf");

		// Try also from a lower level (to aid having the same configuration
		// for many RUN_IN_PLACE installs)
		filenames.push_back(porting::path_user +
				DIR_DELIM + ".." + DIR_DELIM + ".." + DIR_DELIM + "minetest.conf");

		for (const String &filename : filenames) {
			bool r = g_settings.readConfigFile(filename.c_str());
			if (r) {
				g_settings_path = filename;
				break;
			}
		}

		// If no path found, use the first one (menu creates the file)
		if (g_settings_path.empty())
			g_settings_path = filenames[0];
	}

	return true;
}

static void init_log_streams(const Settings &cmd_args)
{
	String log_filename = porting::path_user + DIR_DELIM + DEBUGFILE;

	if (cmd_args.exists("logfile"))
		log_filename = cmd_args.get("logfile");

	g_logger.removeOutput(&file_log_output);
	String conf_loglev = g_settings.get("debug_log_level");

	// Old integer format
	if (std::isdigit(conf_loglev[0])) {
		warningstream << "Deprecated use of debug_log_level with an "
			"integer value; please update your configuration." << std::endl;
		static const char *lev_name[] =
			{"", "error", "action", "info", "verbose", "trace"};
		int lev_i = atoi(conf_loglev.c_str());
		if (lev_i < 0 || lev_i >= (int)ARRLEN(lev_name)) {
			warningstream << "Supplied invalid debug_log_level!"
				"  Assuming action level." << std::endl;
			lev_i = 2;
		}
		conf_loglev = lev_name[lev_i];
	}

	if (log_filename.empty() || conf_loglev.empty())  // No logging
		return;

	LogLevel log_level = Logger::stringToLevel(conf_loglev);
	if (log_level == LL_MAX) {
		warningstream << "Supplied unrecognized debug_log_level; "
			"using maximum." << std::endl;
	}

	file_log_output.setFile(log_filename,
		g_settings.getU64("debug_log_size_max") * 1000000);
	g_logger.addOutputMaxLevel(&file_log_output, log_level);
}

static bool game_configure(GameParams *game_params, const Settings &cmd_args)
{
	game_configure_port(game_params, cmd_args);

	if (!game_configure_world(game_params, cmd_args)) {
		errorstream << "No world path specified or found." << std::endl;
		return false;
	}

	game_configure_subgame(game_params, cmd_args);

	return true;
}

static void game_configure_port(GameParams *game_params, const Settings &cmd_args)
{
	if (cmd_args.exists("port")) {
		game_params.socket_port = cmd_args.getU16("port");
	} else {
		if (game_params.is_dedicated_server)
			game_params.socket_port = g_settings.getU16("port");
		else
			game_params.socket_port = g_settings.getU16("remote_port");
	}

	if (game_params.socket_port == 0)
		game_params.socket_port = DEFAULT_SERVER_PORT;
}

static bool game_configure_world(GameParams *game_params, const Settings &cmd_args)
{
	if (get_world_from_cmdline(game_params, cmd_args))
		return true;

	if (get_world_from_config(game_params, cmd_args))
		return true;

	return auto_select_world(game_params);
}

static bool get_world_from_cmdline(GameParams *game_params, const Settings &cmd_args)
{
	String commanded_world;

	// World name
	String commanded_worldname;
	if (cmd_args.exists("worldname"))
		commanded_worldname = cmd_args.get("worldname");

	// If a world name was specified, convert it to a path
	if (!commanded_worldname.empty()) {
		// Get information about available worlds
		std::vector<WorldSpec> worldspecs = getAvailableWorlds();
		bool found = false;
		for (const WorldSpec &worldspec : worldspecs) {
			String name = worldspec.name;
			if (name == commanded_worldname) {
				dstream << _("Using world specified by --worldname on the "
					"command line") << std::endl;
				commanded_world = worldspec.path;
				found = true;
				break;
			}
		}
		if (!found) {
			dstream << _("World") << " '" << commanded_worldname
			        << _("' not available. Available worlds:") << std::endl;
			print_worldspecs(worldspecs, dstream);
			return false;
		}

		game_params.world_path = get_clean_world_path(commanded_world);
		return !commanded_world.empty();
	}

	if (cmd_args.exists("world"))
		commanded_world = cmd_args.get("world");
	else if (cmd_args.exists("map-dir"))
		commanded_world = cmd_args.get("map-dir");
	else if (cmd_args.exists("nonopt0")) // First nameless argument
		commanded_world = cmd_args.get("nonopt0");

	game_params.world_path = get_clean_world_path(commanded_world);
	return !commanded_world.empty();
}

static bool get_world_from_config(GameParams *game_params, const Settings &cmd_args)
{
	// World directory
	String commanded_world;

	if (g_settings.exists("map-dir"))
		commanded_world = g_settings.get("map-dir");

	game_params.world_path = get_clean_world_path(commanded_world);

	return !commanded_world.empty();
}

static bool auto_select_world(GameParams *game_params)
{
	// No world was specified; try to select it automatically
	// Get information about available worlds

	std::vector<WorldSpec> worldspecs = getAvailableWorlds();
	String world_path;

	// If there is only a single world, use it
	if (worldspecs.size() == 1) {
		world_path = worldspecs[0].path;
		dstream <<_("Automatically selecting world at") << " ["
		        << world_path << "]" << std::endl;
	// If there are multiple worlds, list them
	} else if (worldspecs.size() > 1 && game_params.is_dedicated_server) {
		std::cerr << _("Multiple worlds are available.") << std::endl;
		std::cerr << _("Please select one using --worldname <name>"
				" or --world <path>") << std::endl;
		print_worldspecs(worldspecs, std::cerr);
		return false;
	// If there are no worlds, automatically create a new one
	} else {
		// This is the ultimate default world path
		world_path = porting::path_user + DIR_DELIM + "worlds" +
				DIR_DELIM + "world";
		infostream << "Using default world at ["
		           << world_path << "]" << std::endl;
	}

	assert(!world_path.empty());	// Post-condition
	game_params.world_path = world_path;
	return true;
}

static String get_clean_world_path(const String &path)
{
	const String worldmt = "world.mt";
	String clean_path;

	if (path.size() > worldmt.size()
			&& path.substr(path.size() - worldmt.size()) == worldmt) {
		dstream << _("Supplied world.mt file - stripping it off.") << std::endl;
		clean_path = path.substr(0, path.size() - worldmt.size());
	} else {
		clean_path = path;
	}
	return path;
}


static bool game_configure_subgame(GameParams *game_params, const Settings &cmd_args)
{
	bool success;

	success = get_game_from_cmdline(game_params, cmd_args);
	if (!success)
		success = determine_subgame(game_params);

	return success;
}

static bool get_game_from_cmdline(GameParams *game_params, const Settings &cmd_args)
{
	SubgameSpec commanded_gamespec;

	if (cmd_args.exists("gameid")) {
		String gameid = cmd_args.get("gameid");
		commanded_gamespec = findSubgame(gameid);
		if (!commanded_gamespec.isValid()) {
			errorstream << "Game \"" << gameid << "\" not found" << std::endl;
			return false;
		}
		dstream << _("Using game specified by --gameid on the command line")
		        << std::endl;
		game_params.game_spec = commanded_gamespec;
		return true;
	}

	return false;
}

static bool determine_subgame(GameParams *game_params)
{
	SubgameSpec gamespec;

	assert(!game_params.world_path.empty());	// Pre-condition

	// If world doesn't exist
	if (!game_params.world_path.empty()
		&& !getWorldExists(game_params.world_path)) {
		// Try to take gamespec from command line
		if (game_params.game_spec.isValid()) {
			gamespec = game_params.game_spec;
			infostream << "Using commanded gameid [" << gamespec.id << "]" << std::endl;
		} else {
			if (game_params.is_dedicated_server) {
				// If this is a dedicated server and no gamespec has been specified,
				// print a friendly error pointing to ContentDB.
				errorstream << "To run a " PROJECT_NAME_C " server, you need to select a game using the '--gameid' argument." << std::endl
				            << "Check out https://content.minetest.net for a selection of games to pick from and download." << std::endl;
			}

			return false;
		}
	} else { // World exists
		String world_gameid = getWorldGameId(game_params.world_path, false);
		// If commanded to use a gameid, do so
		if (game_params.game_spec.isValid()) {
			gamespec = game_params.game_spec;
			if (game_params.game_spec.id != world_gameid) {
				warningstream << "Using commanded gameid ["
				            << gamespec.id << "]" << " instead of world gameid ["
				            << world_gameid << "]" << std::endl;
			}
		} else {
			// If world contains an embedded game, use it;
			// Otherwise find world from local system.
			gamespec = findWorldSubgame(game_params.world_path);
			infostream << "Using world gameid [" << gamespec.id << "]" << std::endl;
		}
	}

	if (!gamespec.isValid()) {
		errorstream << "Game [" << gamespec.id << "] could not be found."
		            << std::endl;
		return false;
	}

	game_params.game_spec = gamespec;
	return true;
}


/*****************************************************************************
 * Dedicated server
 *****************************************************************************/
static bool run_dedicated_server(const GameParams &game_params, const Settings &cmd_args)
{
	verbosestream << _("Using world path") << " ["
	              << game_params.world_path << "]" << std::endl;
	verbosestream << _("Using gameid") << " ["
	              << game_params.game_spec.id << "]" << std::endl;

	// Database migration/compression
	if (cmd_args.exists("migrate"))
		return migrate_map_database(game_params, cmd_args);

	if (cmd_args.exists("migrate-players"))
		return ServerEnvironment::migratePlayersDatabase(game_params, cmd_args);

	if (cmd_args.exists("migrate-auth"))
		return ServerEnvironment::migrateAuthDatabase(game_params, cmd_args);

	if (cmd_args.exists("migrate-mod-storage"))
		return Server::migrateModStorageDatabase(game_params, cmd_args);

	if (cmd_args.getFlag("recompress"))
		return recompress_map_database(game_params, cmd_args);

	// Bind address
	String bind_str = g_settings.get("bind_address");
	Address bind_addr(0, 0, 0, 0, game_params.socket_port);

	if (g_settings.getBool("ipv6_server"))
		bind_addr.setAddress(static_cast<IPv6AddressBytes*>(nullptr));
	try {
		bind_addr.Resolve(bind_str.c_str());
	} catch (const ResolveError &e) {
		warningstream << "Resolving bind address \"" << bind_str
			<< "\" failed: " << e.what()
			<< " -- Listening on all addresses." << std::endl;
	}
	if (bind_addr.isIPv6() && !g_settings.getBool("enable_ipv6")) {
		errorstream << "Unable to listen on "
		            << bind_addr.serializeString()
		            << " because IPv6 is disabled" << std::endl;
		return false;
	}

	if (cmd_args.exists("terminal")) {
#if USE_CURSES
		bool name_ok = true;
		String admin_nick = g_settings.get("name");

		name_ok = name_ok && !admin_nick.empty();
		name_ok = name_ok && string_allowed(admin_nick, PLAYERNAME_ALLOWED_CHARS);

		if (!name_ok) {
			if (admin_nick.empty()) {
				errorstream << "No name given for admin. "
					<< "Please check your minetest.conf that it "
					<< "contains a 'name = ' to your main admin account."
					<< std::endl;
			} else {
				errorstream << "Name for admin '"
					<< admin_nick << "' is not valid. "
					<< "Please check that it only contains allowed characters. "
					<< "Valid characters are: " << PLAYERNAME_ALLOWED_CHARS_USER_EXPL
					<< std::endl;
			}
			return false;
		}
		ChatInterface iface;
		bool &kill = *porting::signal_handler_killstatus();

		try {
			// Create server
			Server server(game_params.world_path, game_params.game_spec,
					false, bind_addr, true, &iface);

			g_term_console.setup(&iface, &kill, admin_nick);

			g_term_console.start();

			server.start();
			// Run server
			dedicated_server_loop(server, kill);
		} catch (const ModError &e) {
			g_term_console.stopAndWaitforThread();
			errorstream << "ModError: " << e.what() << std::endl;
			return false;
		} catch (const ServerError &e) {
			g_term_console.stopAndWaitforThread();
			errorstream << "ServerError: " << e.what() << std::endl;
			return false;
		}

		// Tell the console to stop, and wait for it to finish,
		// only then leave context and free iface
		g_term_console.stop();
		g_term_console.wait();

		g_term_console.clearKillStatus();
	} else {
#else
		errorstream << "Cmd arg --terminal passed, but "
			<< "compiled without ncurses. Ignoring." << std::endl;
	} {
#endif
		try {
			// Create server
			Server server(game_params.world_path, game_params.game_spec, false,
				bind_addr, true);
			server.start();

			// Run server
			bool &kill = *porting::signal_handler_killstatus();
			dedicated_server_loop(server, kill);

		} catch (const ModError &e) {
			errorstream << "ModError: " << e.what() << std::endl;
			return false;
		} catch (const ServerError &e) {
			errorstream << "ServerError: " << e.what() << std::endl;
			return false;
		}
	}

	return true;
}

static bool migrate_map_database(const GameParams &game_params, const Settings &cmd_args)
{
	String migrate_to = cmd_args.get("migrate");
	Settings world_mt;
	String world_mt_path = game_params.world_path + DIR_DELIM + "world.mt";
	if (!world_mt.readConfigFile(world_mt_path.c_str())) {
		errorstream << "Cannot read world.mt!" << std::endl;
		return false;
	}

	if (!world_mt.exists("backend")) {
		errorstream << "Please specify your current backend in world.mt:"
			<< std::endl
			<< "	backend = {sqlite3|dummy}"
			<< std::endl;
		return false;
	}

	String backend = world_mt.get("backend");
	if (backend == migrate_to) {
		errorstream << "Cannot migrate: new backend is same"
			<< " as the old one" << std::endl;
		return false;
	}

	MapDatabase *old_db = ServerMap::createDatabase(backend, game_params.world_path, world_mt),
		*new_db = ServerMap::createDatabase(migrate_to, game_params.world_path, world_mt);

	u32 count = 0;
	time_t last_update_time = 0;
	bool &kill = *porting::signal_handler_killstatus();

	std::vector<v3s16> blocks;
	old_db.listAllLoadableBlocks(blocks);
	new_db.beginSave();
	for (std::vector<v3s16>::const_iterator it = blocks.begin(); it != blocks.end(); ++it) {
		if (kill) return false;

		String data;
		old_db.loadBlock(*it, &data);
		if (!data.empty()) {
			new_db.saveBlock(*it, data);
		} else {
			errorstream << "Failed to load block " << *it << ", skipping it." << std::endl;
		}
		if (++count % 0xFF == 0 && time(NULL) - last_update_time >= 1) {
			std::cerr << " Migrated " << count << " blocks, "
				<< (100.0 * count / blocks.size()) << "% completed.\r";
			new_db.endSave();
			new_db.beginSave();
			last_update_time = time(NULL);
		}
	}
	std::cerr << std::endl;
	new_db.endSave();
	delete old_db;
	delete new_db;

	actionstream << "Successfully migrated " << count << " blocks" << std::endl;
	world_mt.set("backend", migrate_to);
	if (!world_mt.updateConfigFile(world_mt_path.c_str()))
		errorstream << "Failed to update world.mt!" << std::endl;
	else
		actionstream << "world.mt updated" << std::endl;

	return true;
}

static bool recompress_map_database(const GameParams &game_params, const Settings &cmd_args)
{
	Settings world_mt;
	const String world_mt_path = game_params.world_path + DIR_DELIM + "world.mt";

	if (!world_mt.readConfigFile(world_mt_path.c_str())) {
		errorstream << "Cannot read world.mt at " << world_mt_path << std::endl;
		return false;
	}
	const String &backend = world_mt.get("backend");
	Server server(game_params.world_path, game_params.game_spec, false, Address(), false);
	MapDatabase *db = ServerMap::createDatabase(backend, game_params.world_path, world_mt);

	u32 count = 0;
	u64 last_update_time = 0;
	bool &kill = *porting::signal_handler_killstatus();
	const u8 serialize_as_ver = SER_FMT_VER_HIGHEST_WRITE;

	// This is ok because the server doesn't actually run
	std::vector<v3s16> blocks;
	db.listAllLoadableBlocks(blocks);
	db.beginSave();
	std::istringstream iss(std::ios_base::binary);
	std::ostringstream oss(std::ios_base::binary);
	for (auto it = blocks.begin(); it != blocks.end(); ++it) {
		if (kill) return false;

		String data;
		db.loadBlock(*it, &data);
		if (data.empty()) {
			errorstream << "Failed to load block " << *it << std::endl;
			return false;
		}

		iss.str(data);
		iss.clear();

		{
			MapBlock mb(v3s16(0,0,0), &server);
			u8 ver = readU8(iss);
			mb.deSerialize(iss, ver, true);

			oss.str("");
			oss.clear();
			writeU8(oss, serialize_as_ver);
			mb.serialize(oss, serialize_as_ver, true, -1);
		}

		db.saveBlock(*it, oss.str());

		count++;
		if (count % 0xFF == 0 && porting::getTimeS() - last_update_time >= 1) {
			std::cerr << " Recompressed " << count << " blocks, "
				<< (100.0f * count / blocks.size()) << "% completed.\r";
			db.endSave();
			db.beginSave();
			last_update_time = porting::getTimeS();
		}
	}
	std::cerr << std::endl;
	db.endSave();

	actionstream << "Done, " << count << " blocks were recompressed." << std::endl;
	return true;
}
