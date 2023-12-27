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

#include <set>
#include <string>
#include <vector>
#include "exceptions.h"

#define DIR_DELIM "/"
#define DIR_DELIM_CHAR '/'
#define FILESYS_CASE_INSENSITIVE false
#define PATH_DELIM ":"

namespace irr::io {
class IFileSystem;
}

namespace fs
{

struct DirListNode
{
	String name;
	bool dir;
};

std::vector<DirListNode> GetDirListing(const String &path);

// Returns true if already exists
bool CreateDir(const String &path);

bool PathExists(const String &path);

bool IsPathAbsolute(const String &path);

bool IsDir(const String &path);

bool IsExecutable(const String &path);

inline bool IsFile(const String &path)
{
	return PathExists(path) && !IsDir(path);
}

bool IsDirDelimiter(char c);

// Only pass full paths to this one. True on success.
// NOTE: The WIN32 version returns always true.
bool RecursiveDelete(const String &path);

bool DeleteSingleFileOrEmptyDirectory(const String &path);

// Returns path to temp directory, can return "" on error
String TempPath();

// Returns path to securely-created temporary file (will already exist when this function returns)
// can return "" on error
String CreateTempFile();

/* Returns a list of subdirectories, including the path itself, but excluding
       hidden directories (whose names start with . or _)
*/
void GetRecursiveDirs(std::vector<String> &dirs, const String &dir);
std::vector<String> GetRecursiveDirs(const String &dir);

/* Multiplatform */

/* The path itself not included, returns a list of all subpaths.
   dst - vector that contains all the subpaths.
   list files - include files in the list of subpaths.
   ignore - paths that start with these charcters will not be listed.
*/
void GetRecursiveSubPaths(const String &path,
		  std::vector<String> &dst,
		  bool list_files,
		  const std::set<char> &ignore = {});

// Only pass full paths to this one. True on success.
bool RecursiveDeleteContent(const String &path);

// Create all directories on the given path that don't already exist.
bool CreateAllDirs(const String &path);

// Copy a regular file
bool CopyFileContents(const String &source, const String &target);

// Copy directory and all subdirectories
// Omits files and subdirectories that start with a period
bool CopyDir(const String &source, const String &target);

// Move directory and all subdirectories
// Behavior with files/subdirs that start with a period is undefined
bool MoveDir(const String &source, const String &target);

// Check if one path is prefix of another
// For example, "/tmp" is a prefix of "/tmp" and "/tmp/file" but not "/tmp2"
// Ignores case differences and '/' vs. '\\' on Windows
bool PathStartsWith(const String &path, const String &prefix);

// Remove last path component and the dir delimiter before and/or after it,
// returns "" if there is only one path component.
// removed: If non-NULL, receives the removed component(s).
// count: Number of components to remove
String RemoveLastPathComponent(const String &path,
		String *removed = NULL, int count = 1);

// Remove "." and ".." path components and for every ".." removed, remove
// the last normal path component before it. Unlike AbsolutePath,
// this does not resolve symlinks and check for existence of directories.
String RemoveRelativePathComponents(String path);

// Returns the absolute path for the passed path, with "." and ".." path
// components and symlinks removed.  Returns "" on error.
String AbsolutePath(const String &path);

// Returns the filename from a path or the entire path if no directory
// delimiter is found.
const char *GetFilenameFromPath(const char *path);

bool safeWriteToFile(const String &path, const String &content);

bool ReadFile(const String &path, String &out);

bool Rename(const String &from, const String &to);

} // namespace fs
