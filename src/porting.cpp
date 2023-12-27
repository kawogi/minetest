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

/*
	Random portability stuff

	See comments in porting.h
*/

#include "porting.h"

#if defined(__FreeBSD__)  || defined(__NetBSD__) || defined(__DragonFly__) || defined(__OpenBSD__)
	#include <sys/types.h>
	#include <sys/sysctl.h>
	extern char **environ;
#endif
#include <unistd.h>
#include <sys/utsname.h>
#include <spawn.h>
#if defined(__hpux)
	#define _PSTAT64
	#include <sys/pstat.h>
#endif

#if defined(__HAIKU__)
	#include <FindDirectory.h>
#endif

#include "config.h"
#include "debug.h"
#include "filesys.h"
#include "log.h"
#include "util/string.h"
#include <list>
#include <cstdarg>
#include <cstdio>

namespace porting
{

/*
	Signal handler (grabs Ctrl-C on POSIX systems)
*/

bool g_killed = false;

bool *signal_handler_killstatus()
{
	return &g_killed;
}

#include <signal.h>

void signal_handler(int sig)
{
	if (!g_killed) {
		if (sig == SIGINT) {
			dstream << "INFO: signal_handler(): "
				<< "Ctrl-C pressed, shutting down." << std::endl;
		} else if (sig == SIGTERM) {
			dstream << "INFO: signal_handler(): "
				<< "got SIGTERM, shutting down." << std::endl;
		}

		// Comment out for less clutter when testing scripts
		/*dstream << "INFO: sigint_handler(): "
				<< "Printing debug stacks" << std::endl;
		debug_stacks_print();*/

		g_killed = true;
	} else {
		(void)signal(sig, SIG_DFL);
	}
}

void signal_handler_init(void)
{
	(void)signal(SIGINT, signal_handler);
	(void)signal(SIGTERM, signal_handler);
}

/*
	Path mangler
*/

// Default to RUN_IN_PLACE style relative paths
String path_share = "..";
String path_user = "..";
String path_locale = path_share + DIR_DELIM + "locale";
String path_cache = path_user + DIR_DELIM + "cache";


String getDataPath(const char *subpath)
{
	return path_share + DIR_DELIM + subpath;
}

void pathRemoveFile(char *path, char delim)
{
	// Remove filename and path delimiter
	int i;
	for(i = strlen(path)-1; i>=0; i--)
	{
		if(path[i] == delim)
			break;
	}
	path[i] = 0;
}

bool detectMSVCBuildDir(const String &path)
{
	const char *ends[] = {
		"bin\\Release",
		"bin\\MinSizeRel",
		"bin\\RelWithDebInfo",
		"bin\\Debug",
		"bin\\Build",
		NULL
	};
	return (!removeStringEnd(path, ends).empty());
}

String get_sysinfo()
{
	struct utsname osinfo;
	uname(&osinfo);
	return String(osinfo.sysname) + "/"
		+ osinfo.release + " " + osinfo.machine;
}


bool getCurrentWorkingDir(char *buf, size_t len)
{
	return getcwd(buf, len);
}


bool getExecPathFromProcfs(char *buf, size_t buflen)
{
	buflen--;

	ssize_t len;
	if ((len = readlink("/proc/self/exe",     buf, buflen)) == -1 &&
		(len = readlink("/proc/curproc/file", buf, buflen)) == -1 &&
		(len = readlink("/proc/curproc/exe",  buf, buflen)) == -1)
		return false;

	buf[len] = '\0';
	return true;
}

//// Windows
#if defined(__linux__)

bool getCurrentExecPath(char *buf, size_t len)
{
	if (!getExecPathFromProcfs(buf, len))
		return false;

	return true;
}

//// FreeBSD, NetBSD, DragonFlyBSD
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__DragonFly__)

bool getCurrentExecPath(char *buf, size_t len)
{
	// Try getting path from procfs first, since valgrind
	// doesn't work with the latter
	if (getExecPathFromProcfs(buf, len))
		return true;

	int mib[4];

	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_PATHNAME;
	mib[3] = -1;

	if (sysctl(mib, 4, buf, &len, NULL, 0) == -1)
		return false;

	return true;
}

#elif defined(__HAIKU__)

bool getCurrentExecPath(char *buf, size_t len)
{
	return find_path(B_APP_IMAGE_SYMBOL, B_FIND_PATH_IMAGE_PATH, NULL, buf, len) == B_OK;
}

//// Solaris
#elif defined(__sun) || defined(sun)

bool getCurrentExecPath(char *buf, size_t len)
{
	const char *exec = getexecname();
	if (exec == NULL)
		return false;

	if (strlcpy(buf, exec, len) >= len)
		return false;

	return true;
}


// HP-UX
#elif defined(__hpux)

bool getCurrentExecPath(char *buf, size_t len)
{
	struct pst_status psts;

	if (pstat_getproc(&psts, sizeof(psts), 0, getpid()) == -1)
		return false;

	if (pstat_getpathname(buf, len, &psts.pst_fid_text) == -1)
		return false;

	return true;
}


#else

bool getCurrentExecPath(char *buf, size_t len)
{
	return false;
}

#endif


//// Non-Windows
const char *getHomeOrFail()
{
	const char *home = getenv("HOME");
	// In rare cases the HOME environment variable may be unset
	FATAL_ERROR_IF(!home,
		"Required environment variable HOME is not set");
	return home;
}


//// Linux
#if defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__DragonFly__)

bool setSystemPaths()
{
	char buf[BUFSIZ];

	if (!getCurrentExecPath(buf, sizeof(buf))) {
		FATAL_ERROR("Unable to read bindir");
		return false;
	}

	pathRemoveFile(buf, '/');
	String bindir(buf);

	// Find share directory from these.
	// It is identified by containing the subdirectory "builtin".
	std::list<String> trylist;
	String static_sharedir = STATIC_SHAREDIR;
	if (!static_sharedir.empty() && static_sharedir != ".")
		trylist.push_back(static_sharedir);

	trylist.push_back(bindir + DIR_DELIM ".." DIR_DELIM "share"
		DIR_DELIM + PROJECT_NAME);
	trylist.push_back(bindir + DIR_DELIM "..");

	for (std::list<String>::const_iterator
			i = trylist.begin(); i != trylist.end(); ++i) {
		const String &trypath = *i;
		if (!fs::PathExists(trypath) ||
			!fs::PathExists(trypath + DIR_DELIM + "builtin")) {
			warningstream << "system-wide share not found at \""
					<< trypath << "\""<< std::endl;
			continue;
		}

		// Warn if was not the first alternative
		if (i != trylist.begin()) {
			warningstream << "system-wide share found at \""
					<< trypath << "\"" << std::endl;
		}

		path_share = trypath;
		break;
	}

	const char *const minetest_user_path = getenv("MINETEST_USER_PATH");
	if (minetest_user_path && minetest_user_path[0] != '\0') {
		path_user = String(minetest_user_path);
	} else {
		path_user = String(getHomeOrFail()) + DIR_DELIM "."
			+ PROJECT_NAME;
	}

	return true;
}

#else

bool setSystemPaths()
{
	path_share = STATIC_SHAREDIR;
	const char *const minetest_user_path = getenv("MINETEST_USER_PATH");
	if (minetest_user_path && minetest_user_path[0] != '\0') {
		path_user = String(minetest_user_path);
	} else {
		path_user  = String(getHomeOrFail()) + DIR_DELIM "."
			+ lowercase(PROJECT_NAME);
	}
	return true;
}


#endif

void migrateCachePath()
{
	const String local_cache_path = path_user + DIR_DELIM + "cache";

	// Delete tmp folder if it exists (it only ever contained
	// a temporary ogg file, which is no longer used).
	if (fs::PathExists(local_cache_path + DIR_DELIM + "tmp"))
		fs::RecursiveDelete(local_cache_path + DIR_DELIM + "tmp");

	// Bail if migration impossible
	if (path_cache == local_cache_path || !fs::PathExists(local_cache_path)
			|| fs::PathExists(path_cache)) {
		return;
	}
	if (!fs::Rename(local_cache_path, path_cache)) {
		errorstream << "Failed to migrate local cache path "
			"to system path!" << std::endl;
	}
}

void initializePaths()
{
#if RUN_IN_PLACE
	char buf[BUFSIZ];

	infostream << "Using relative paths (RUN_IN_PLACE)" << std::endl;

	bool success =
		getCurrentExecPath(buf, sizeof(buf)) ||
		getExecPathFromProcfs(buf, sizeof(buf));

	if (success) {
		pathRemoveFile(buf, DIR_DELIM_CHAR);
		String execpath(buf);

		path_share = execpath + DIR_DELIM "..";
		path_user  = execpath + DIR_DELIM "..";

		if (detectMSVCBuildDir(execpath)) {
			path_share += DIR_DELIM "..";
			path_user  += DIR_DELIM "..";
		}
	} else {
		errorstream << "Failed to get paths by executable location, "
			"trying cwd" << std::endl;

		if (!getCurrentWorkingDir(buf, sizeof(buf)))
			FATAL_ERROR("Ran out of methods to get paths");

		size_t cwdlen = strlen(buf);
		if (cwdlen >= 1 && buf[cwdlen - 1] == DIR_DELIM_CHAR) {
			cwdlen--;
			buf[cwdlen] = '\0';
		}

		if (cwdlen >= 4 && !strcmp(buf + cwdlen - 4, DIR_DELIM "bin"))
			pathRemoveFile(buf, DIR_DELIM_CHAR);

		String execpath(buf);

		path_share = execpath;
		path_user  = execpath;
	}
	path_cache = path_user + DIR_DELIM + "cache";
#else
	infostream << "Using system-wide paths (NOT RUN_IN_PLACE)" << std::endl;

	if (!setSystemPaths())
		errorstream << "Failed to get one or more system-wide path" << std::endl;


	// Initialize path_cache
	// First try $XDG_CACHE_HOME/PROJECT_NAME
	const char *cache_dir = getenv("XDG_CACHE_HOME");
	const char *home_dir = getenv("HOME");
	if (cache_dir && cache_dir[0] != '\0') {
		path_cache = String(cache_dir) + DIR_DELIM + PROJECT_NAME;
	} else if (home_dir) {
		// Then try $HOME/.cache/PROJECT_NAME
		path_cache = String(home_dir) + DIR_DELIM + ".cache"
			+ DIR_DELIM + PROJECT_NAME;
	} else {
		// If neither works, use $PATH_USER/cache
		path_cache = path_user + DIR_DELIM + "cache";
	}
	// Migrate cache folder to new location if possible
	migrateCachePath();
#endif // RUN_IN_PLACE

	infostream << "Detected share path: " << path_share << std::endl;
	infostream << "Detected user path: " << path_user << std::endl;
	infostream << "Detected cache path: " << path_cache << std::endl;

#if USE_GETTEXT
	bool found_localedir = false;
#  ifdef STATIC_LOCALEDIR
	/* STATIC_LOCALEDIR may be a generalized path such as /usr/share/locale that
	 * doesn't necessarily contain our locale files, so check data path first. */
	path_locale = getDataPath("locale");
	if (fs::PathExists(path_locale)) {
		found_localedir = true;
		infostream << "Using in-place locale directory " << path_locale
			<< " even though a static one was provided." << std::endl;
	} else if (STATIC_LOCALEDIR[0] && fs::PathExists(STATIC_LOCALEDIR)) {
		found_localedir = true;
		path_locale = STATIC_LOCALEDIR;
		infostream << "Using static locale directory " << STATIC_LOCALEDIR
			<< std::endl;
	}
#  else
	path_locale = getDataPath("locale");
	if (fs::PathExists(path_locale)) {
		found_localedir = true;
	}
#  endif
	if (!found_localedir) {
		warningstream << "Couldn't find a locale directory!" << std::endl;
	}
#endif  // USE_GETTEXT
}

////
//// OS-specific Secure Random
////

#ifdef WIN32

bool secure_rand_fill_buf(void *buf, size_t len)
{
	HCRYPTPROV wctx;

	if (!CryptAcquireContext(&wctx, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
		return false;

	CryptGenRandom(wctx, len, (BYTE *)buf);
	CryptReleaseContext(wctx, 0);
	return true;
}

#else

bool secure_rand_fill_buf(void *buf, size_t len)
{
	// N.B.  This function checks *only* for /dev/urandom, because on most
	// common OSes it is non-blocking, whereas /dev/random is blocking, and it
	// is exceptionally uncommon for there to be a situation where /dev/random
	// exists but /dev/urandom does not.  This guesswork is necessary since
	// random devices are not covered by any POSIX standard...
	FILE *fp = fopen("/dev/urandom", "rb");
	if (!fp)
		return false;

	bool success = fread(buf, len, 1, fp) == 1;

	fclose(fp);
	return success;
}

#endif

void attachOrCreateConsole()
{
}

int mt_snprintf(char *buf, const size_t buf_size, const char *fmt, ...)
{
	// https://msdn.microsoft.com/en-us/library/bt7tawza.aspx
	//  Many of the MSVC / Windows printf-style functions do not support positional
	//  arguments (eg. "%1$s"). We just forward the call to vsnprintf for sane
	//  platforms, but defer to _vsprintf_p on MSVC / Windows.
	// https://github.com/FFmpeg/FFmpeg/blob/5ae9fa13f5ac640bec113120d540f70971aa635d/compat/msvcrt/snprintf.c#L46
	//  _vsprintf_p has to be shimmed with _vscprintf_p on -1 (for an example see
	//  above FFmpeg link).
	va_list args;
	va_start(args, fmt);
	int c = vsnprintf(buf, buf_size, fmt, args);
	va_end(args);
	return c;
}

static bool open_uri(const String &uri)
{
	if (uri.find_first_of("\r\n") != String::npos) {
		errorstream << "Unable to open URI as it is invalid, contains new line: " << uri << std::endl;
		return false;
	}

	const char *argv[] = {"xdg-open", uri.c_str(), NULL};
	return posix_spawnp(NULL, "xdg-open", NULL, NULL, (char**)argv, environ) == 0;
}

bool open_url(const String &url)
{
	if (url.substr(0, 7) != "http://" && url.substr(0, 8) != "https://") {
		errorstream << "Unable to open browser as URL is missing schema: " << url << std::endl;
		return false;
	}

	return open_uri(url);
}

bool open_directory(const String &path)
{
	if (!fs::IsDir(path)) {
		errorstream << "Unable to open directory as it does not exist: " << path << std::endl;
		return false;
	}

	return open_uri(path);
}

} //namespace porting
